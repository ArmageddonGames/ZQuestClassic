import dataclasses
import heapq
import json
import logging
import os
import platform
import shutil
import subprocess
import sys
import traceback

from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path
from time import sleep
from timeit import default_timer as timer
from typing import Any, Callable, Dict, Generator, List, Literal, Optional, Tuple, Union

from lib.replay_helpers import parse_result_txt_file, read_replay_meta
from watchdog.events import FileSystemEventHandler
from watchdog.observers import Observer

script_dir = Path(os.path.dirname(os.path.realpath(__file__)))
root_dir = script_dir.parent
replays_dir = script_dir / 'replays'
is_ci = 'CI' in os.environ


# TODO: this is a duplicated function
def get_channel():
    system = platform.system()
    if system == 'Darwin':
        return 'mac'
    elif system == 'Windows':
        return 'windows'
    elif system == 'Linux':
        return 'linux'
    else:
        raise Exception(f'unexpected system: {system}')


ASSERT_FAILED_EXIT_CODE = 120

estimate_divisor = 1


def configure_estimate_multiplier(build_folder: Path, build_type: str):
    global estimate_divisor
    channel = get_channel()
    is_ci = 'CI' in os.environ
    is_mac_ci = is_ci and channel == 'mac'
    is_web = 'build_emscripten' in str(build_folder)
    is_web_ci = is_web and is_ci
    is_coverage = build_folder.name == 'Coverage' or build_type == 'Coverage'
    is_asan = build_folder.name == 'Asan' or build_type == 'Asan'

    m = 1
    if is_mac_ci:
        m *= 2
    if is_web_ci:
        m *= 30
    elif is_web:
        m *= 11
    if is_coverage:
        m *= 10
    if is_asan:
        m *= 15
    estimate_divisor = m


@dataclass
class Replay:
    name: str
    path: Path
    meta: dict
    frames: int

    frame_arg: Optional[int] = None
    snapshot_arg: Optional[str] = None

    def frames_limited(self):
        if self.frame_arg != None:
            return self.frame_arg
        return self.frames

    def estimated_fps(self):
        return estimate_fps(self)

    def estimated_duration(self):
        return self.frames_limited() / self.estimated_fps()


@dataclass
class RunResult:
    name: str
    directory: str
    path: str
    success: bool = False
    stopped: bool = False
    exit_code: int = None
    duration: float = None
    fps: int = None
    frame: int = None
    num_frames: int = None
    failing_frame: int = None
    unexpected_gfx_frames: List[int] = None
    unexpected_gfx_segments: List[Tuple[int, int]] = None
    unexpected_gfx_segments_limited: List[Tuple[int, int]] = None
    exceptions: List[str] = field(default_factory=list)
    # Only for compare report.
    snapshots: List[Any] = None


@dataclass
class ReplayTestResults:
    runs_on: Literal['windows-2022', 'macos-13', 'ubuntu-22.04']
    arch: Literal['x64', 'win32', 'intel']
    ci: bool
    workflow_run_id: Optional[int]
    git_ref: Optional[str]
    zc_version: str
    time: str
    runs: List[List[RunResult]]
    # Only for compare report.
    label: str = None

    def __post_init__(self):
        if self.runs and isinstance(self.runs[0][0], dict):
            deserialized = []
            for runs in self.runs:
                for run in runs:
                    # Old property, will error when creating dataclass.
                    run.pop('diff', None)
                # New property.
                if 'path' not in run:
                    run['path'] = ''
                deserialized.append([RunResult(**run) for run in runs])
            self.runs = deserialized

    def to_json(self, indent=2):
        as_dict = dataclasses.asdict(self)
        return json.dumps(as_dict, indent=indent)


class ReplayResultUpdatedHandler(FileSystemEventHandler):

    def __init__(self, path, callback=None):
        self.path = path
        self.callback = callback
        self.result = None
        self.is_result_stale = False
        self.observer = Observer()
        self.observer.schedule(self, path.parent, recursive=False)
        self.observer.start()

    def update_result(self):
        if self.is_result_stale:
            self.parse_result()

    def parse_result(self):
        if self.result and self.result['stopped']:
            return

        if not self.path.exists():
            return

        try:
            new_result = parse_result_txt_file(self.path)
            if not new_result:
                return
            self.result = new_result
        except:
            logging.warning('could not read result txt file')
            return

        self.is_result_stale = False
        if self.result['stopped']:
            self.observer.stop()

    def on_modified(self, event):
        if not event.is_directory and event.src_path.endswith(self.path.name):
            self.modified_time = timer()
            self.is_result_stale = True
            if self.callback:
                self.callback(self)

    def on_created(self, event):
        if not event.is_directory and event.src_path.endswith(self.path.name):
            self.modified_time = timer()
            self.is_result_stale = True
            if self.callback:
                self.callback(self)


class ReplayTimeoutException(Exception):
    pass


def get_arg_for_replay(replay_file, grouped_arg, is_int=False):
    arg_by_replay, default_arg = grouped_arg
    if replay_file in arg_by_replay:
        result = arg_by_replay[replay_file]
    else:
        result = default_arg

    if is_int and result != None:
        return int(result)
    return result


def estimate_fps(replay: Replay):
    # Based on speed found on Windows 64-bit in CI. Should be manually updated occasionally.
    # NOTE: this are w/o any concurrency, so real numbers in CI today are expected to be lower. Maybe just remove this?
    fps = 1500
    estimated_fps_overrides = {
        'quests/Z1 Recreations/classic_1st.qst': 3000,
        'demosp253.qst': 1600,
        'dreamy_cambria.qst': 1100,
        'first_quest_layered.qst': 2500,
        'freedom_in_chains.qst': 1300,
        'hell_awaits.qst': 2800,
        'hero_of_dreams.qst': 2400,
        'hollow_forest.qst': 500,
        'lands_of_serenity.qst': 1700,
        'link_to_the_zelda.qst': 2100,
        'nes_remastered.qst': 3000,
        'new2013.qst': 2800,
        'ss_jenny.qst': 1500,
        'stellar_seas_randomizer.qst': 500,
        'yuurand.qst': 650,
    }
    qst = replay.meta['qst']
    if qst in estimated_fps_overrides:
        fps = estimated_fps_overrides[qst]

    return fps / estimate_divisor


def apply_test_filter(tests: List[Path], filter: str):
    filter_as_path = Path(filter)

    exact_match = next((t for t in tests if t == filter_as_path.absolute()), None)
    if exact_match:
        return [exact_match]

    exact_match = next((t for t in tests if t == replays_dir / filter_as_path), None)
    if exact_match:
        return [exact_match]

    if filter_as_path.is_relative_to(replays_dir):
        filter_as_path = filter_as_path.relative_to(replays_dir)

    filtered = []
    for test in tests:
        rel = test.relative_to(replays_dir)
        if filter_as_path.as_posix() in rel.as_posix():
            filtered.append(test)
    return filtered


def time_format(ms: int):
    if ms is not None:
        seconds = int(ms / 1000)
        m = seconds // 60
        s = seconds % 60
        if m > 0:
            return '{}m {:02d}s'.format(m, s)
        elif s >= 0:
            return '{}s'.format(s)
    return '-'


def get_replay_name(replay_file: Path, replays_dir: Path):
    if replay_file.is_relative_to(replays_dir):
        return replay_file.relative_to(replays_dir).as_posix()
    else:
        return replay_file.name


def load_replays(replay_paths: List[Path], relative_to: Path) -> List[Replay]:
    replays = []
    for path in replay_paths:
        if not path.exists():
            raise Exception(f'file does not exist: {path}')
        if not path.is_file():
            raise Exception(f'is not a file: {path}')

        name = get_replay_name(path, relative_to)
        meta = read_replay_meta(path)
        frames = int(meta['frames'])

        if meta['qst'] == 'playground.qst' and meta['version'] != 'latest':
            raise Exception(
                f'all playground.qst replays must set version to "latest": {path}'
            )
        replay = Replay(
            name=name,
            path=path,
            meta=meta,
            frames=frames,
        )
        replays.append(replay)

    return replays


@dataclass
class RunReplayTestsContext:
    test_results: ReplayTestResults
    mode: str
    arch: str
    replays: List[Replay]
    build_folder: Path
    test_results_dir: Path
    runs_dir: Path
    concurrency: int
    timeout: bool
    debugger: bool
    headless: bool
    jit: bool
    extra_args: List[str]


@dataclass
class StartReplayArgs:
    ctx: RunReplayTestsContext
    replay: Replay
    output_dir: Path


class CLIPlayerInterface:
    p = None

    def start_replay(self, args: StartReplayArgs):
        self.p = None
        ctx = args.ctx
        replay = args.replay
        replay_path = args.replay.path
        output_dir = args.output_dir
        extra_args = args.ctx.extra_args

        # TODO: fix this common-ish error, and whatever else is causing random failures.
        # Assertion failed: (mutex), function al_lock_mutex, file threads.Assertion failed: (mutex), function al_lock_mutex, file threads.c, line 324.
        # Assertion failed: (mutex), function al_lock_mutex, file threads.c, line 324.
        exe_path = ctx.build_folder / ('zplayer.exe' if os.name == 'nt' else 'zplayer')
        if not exe_path.exists():
            exe_path = ctx.build_folder / ('zelda.exe' if os.name == 'nt' else 'zelda')
        if not exe_path.exists():
            print(
                f'could not find executable at: {ctx.build_folder}\nYou may need to set the --build_folder arg (defaults to build/Release)'
            )
            os._exit(1)

        exe_args = [
            exe_path.absolute(),
            f'-{args.ctx.mode}',
            replay_path,
            '-replay-exit-when-done',
            '-replay-output-dir',
            output_dir,
            '-script-runtime-debug-folder',
            str(output_dir / 'zscript-debug'),
            # TODO !!
            # '-optimize-zasm',
        ]

        if extra_args:
            exe_args.extend(extra_args)

        if ctx.debugger:
            exe_args = [
                sys.executable,
                root_dir / 'scripts/run_target.py',
                exe_path.stem,
            ] + exe_args[1:]

        if replay.snapshot_arg is not None:
            exe_args.extend(['-snapshot', replay.snapshot_arg])

        if replay.frame_arg != None:
            exe_args.extend(['-frame', str(replay.frame_arg)])

        if ctx.jit:
            exe_args.append('-jit')
            exe_args.append('-jit-log')
            exe_args.append('-jit-fatal-compile-errors')

        if ctx.headless:
            exe_args.append('-headless')

        allegro_log_path = output_dir / 'allegro.log'
        self.p = subprocess.Popen(
            exe_args,
            cwd=ctx.build_folder,
            env={
                **os.environ,
                'ALLEGRO_LEGACY_TRACE': str(allegro_log_path),
                'BUILD_FOLDER': str(ctx.build_folder.absolute()),
            },
            stdout=open(output_dir / 'stdout.txt', 'w'),
            stderr=open(output_dir / 'stderr.txt', 'w'),
            encoding='utf-8',
            text=True,
        )

    def wait_for_finish(self):
        if not self.p:
            return

        self.p.wait()

    def get_exit_code(self):
        if not self.p:
            return -1

        return self.p.returncode

    def poll(self):
        if not self.p:
            return

        return self.p.poll()

    def stop(self):
        if not self.p:
            return

        self.p.terminate()
        self.p.wait()


class WebPlayerInterface:
    p = None

    def start_replay(self, args: StartReplayArgs):
        self.p = None
        ctx = args.ctx
        replay = args.replay
        replay_path = args.replay.path
        output_dir = args.output_dir

        if replay_path.is_relative_to(root_dir / 'tests/replays'):
            rel_replay_path = replay_path.relative_to(root_dir / 'tests/replays')
        else:
            # Not one of our normal replays - so copy it over.
            rel_replay_path = 'tmp.zplay'
            shutil.copy(
                replay_path,
                ctx.build_folder / 'packages/web/files/test_replays' / rel_replay_path,
            )

        url = f'play/?{ctx.mode}=test_replays/{rel_replay_path}'

        extra_args = [
            '-replay-exit-when-done',
            '-show-fps',
            '-optimize-zasm',
        ]
        if ctx.extra_args:
            extra_args.extend(ctx.extra_args)

        if ctx.headless:
            extra_args.append('-headless')

        if replay.snapshot_arg is not None:
            extra_args.append(f'-snapshot {replay.snapshot_arg}')

        if replay.frame_arg != None:
            extra_args.append(f'-frame {replay.frame_arg}')

        if ctx.jit:
            extra_args.append('-jit')
            extra_args.append('-jit-log')
            extra_args.append('-jit-fatal-compile-errors')

        exe_args = [
            'node',
            root_dir / 'web/tests/run_replay.js',
            'http://localhost:8000',
            output_dir,
            url,
            ' '.join(extra_args),
            output_dir / replay_path.with_suffix('.zplay.result.txt').name,
        ]

        self.p = subprocess.Popen(
            exe_args,
            stdout=open(output_dir / 'stdout.txt', 'w'),
            stderr=open(output_dir / 'stderr.txt', 'w'),
        )

    def wait_for_finish(self):
        if not self.p:
            return

        self.p.wait()

    def get_exit_code(self):
        if not self.p:
            return -1

        return self.p.returncode

    def poll(self):
        if not self.p:
            return

        return self.p.poll()

    def stop(self):
        if not self.p:
            return

        self.p.terminate()
        self.p.wait()


RunReplayTestGenerator = Generator[Tuple[int, str, RunResult], None, None]


def _run_replay_test(
    ctx: RunReplayTestsContext, key: int, replay: Replay, output_dir: Path
) -> RunReplayTestGenerator:
    test_results_dir = ctx.test_results_dir
    replay_file = replay.path
    name = replay.name
    directory = output_dir.relative_to(test_results_dir).as_posix()
    result = RunResult(name=name, directory=directory, path=replay_file.as_posix())
    roundtrip_path = output_dir / f'{name}.roundtrip'
    allegro_log_path = output_dir / 'allegro.log'
    result_path = output_dir / replay_file.with_suffix('.zplay.result.txt').name
    do_timeout = ctx.timeout

    timeout = 60
    if replay_file.name == 'yuurand.zplay':
        timeout = 180

    if 'build_emscripten' in str(ctx.build_folder):
        player_interface = WebPlayerInterface()
    else:
        player_interface = CLIPlayerInterface()

    max_start_attempts = 5
    for _ in range(0, max_start_attempts):
        watcher = None
        try:
            if result_path.exists():
                result_path.unlink()
            if roundtrip_path.exists():
                roundtrip_path.unlink()
            if allegro_log_path.exists():
                allegro_log_path.unlink()

            def on_result_updated(w: ReplayResultUpdatedHandler):
                if not w:
                    return

                w.update_result()
                if not w.result:
                    return

                result.duration = w.result['duration']
                result.fps = int(w.result['fps'])
                result.frame = int(w.result['frame'])
                result.num_frames = int(w.result['replay_log_frames'])
                result.stopped = w.result['stopped']
                result.success = w.result['stopped'] and w.result['success']

            watcher = ReplayResultUpdatedHandler(result_path, on_result_updated)

            start = timer()
            player_interface.start_replay(
                StartReplayArgs(
                    ctx=ctx,
                    replay=replay,
                    output_dir=output_dir,
                )
            )

            # Wait for .zplay.result.txt creation.
            while True:
                if do_timeout and timer() - start > timeout:
                    raise ReplayTimeoutException(
                        'timed out waiting for replay to start'
                    )

                watcher.update_result()
                if watcher.result:
                    break

                retcode = player_interface.poll()
                if retcode != None:
                    watcher.update_result()
                    if not watcher.result:
                        raise Exception(
                            f'process finished before replay started, exit code: {retcode}'
                        )

                yield (key, 'status', result)

            if 'zc_version' in watcher.result:
                ctx.test_results.zc_version = watcher.result['zc_version']

            # .zplay.result.txt should be updated every second.
            while watcher.observer.is_alive():
                watcher.update_result()

                if player_interface.poll() != None:
                    break

                # Don't apply timeout until beyond the first frame, since JIT may take a moment for big scripts.
                if watcher.result['frame'] == 0:
                    continue

                if do_timeout and timer() - watcher.modified_time > timeout:
                    last_frame = watcher.result['frame']
                    raise ReplayTimeoutException(
                        f'timed out, replay got stuck around frame {last_frame}'
                    )

                yield (key, 'status', result)

            player_interface.wait_for_finish()
            on_result_updated(watcher)

            if not result.success:
                result.failing_frame = watcher.result.get('failing_frame', None)
                result.unexpected_gfx_frames = watcher.result.get(
                    'unexpected_gfx_frames', None
                )
                result.unexpected_gfx_segments = watcher.result.get(
                    'unexpected_gfx_segments', None
                )
                result.unexpected_gfx_segments_limited = watcher.result.get(
                    'unexpected_gfx_segments_limited', None
                )
            else:
                result.failing_frame = None
                result.unexpected_gfx_frames = None
                result.unexpected_gfx_segments = None
                result.unexpected_gfx_segments_limited = None
            exit_code = player_interface.get_exit_code()
            result.exit_code = exit_code
            if exit_code != 0 and exit_code != ASSERT_FAILED_EXIT_CODE:
                result.exceptions.append(
                    f'replay failed with unexpected code {exit_code}'
                )
            # .zplay files are updated in-place, but lets also copy over to the test output folder.
            # This makes it easy to upload an archive of updated replays in CI.
            if ctx.mode == 'update' and watcher.result['changed']:
                (test_results_dir / 'updated').mkdir(exist_ok=True)
                shutil.copy2(
                    replay_file, test_results_dir / 'updated' / replay_file.name
                )
            break
        except ReplayTimeoutException as e:
            # Will try again.
            result.exceptions.append(str(e))
            player_interface.stop()
        except KeyboardInterrupt:
            exit(1)
        except GeneratorExit:
            return
        except BaseException as e:
            e = ''.join(traceback.TracebackException.from_exception(e).format())
            result.exceptions.append(e)
            on_result_updated(watcher)
            yield (key, 'finish', result)
            return
        finally:
            if watcher:
                watcher.observer.stop()
            if player_interface:
                player_interface.wait_for_finish()

    yield (key, 'finish', result)


@dataclass
class RunReplayTestsProgress:
    all_results: ReplayTestResults
    results: RunResult
    active: List[RunResult]
    pending: List[Replay]
    status: Dict[str, str]
    eta: str
    summary: str


def _run_replays(
    ctx: RunReplayTestsContext, on_update: Callable[[], RunReplayTestsProgress]
) -> RunResult:
    replays = ctx.replays
    concurrency = ctx.concurrency
    runs_dir = ctx.runs_dir

    results: List[RunResult] = []
    pending_replays = [*replays]
    active_tests: List[RunReplayTestGenerator] = []
    active_results = []
    replays_by_name = {r.name: r for r in replays}

    def get_eta():
        if not active_results:
            return ''

        # https://stackoverflow.com/a/49721962/2788187
        def compute_time(tasks, X):
            threads = [0] * X
            for t in tasks:
                heapq.heappush(threads, heapq.heappop(threads) + t)
            return max(threads)

        frames_so_far = sum(r.frame for r in results if r.frame) + sum(
            r.frame for r in active_results if r.frame
        )
        durs_so_far = (
            sum(r.duration for r in results if r.frame)
            + sum(r.duration for r in active_results if r.frame)
        ) / 1000
        avg_fps = frames_so_far / durs_so_far if durs_so_far else 0
        avg_fps_established = frames_so_far > 5000 and avg_fps
        if not avg_fps_established:
            return 'ETA ...'

        durs = []

        # For actives tests, divide the frames remaining by the running average FPS.
        # If average FPS has not been established yet, use the rough estimate FPS.
        for result in active_results:
            replay = replays_by_name[result.name]
            if result.frame and result.num_frames and result.fps:
                if result.frame > 10000 or result.frame / result.num_frames > 0.1:
                    # Some replays tend to get much slower as they go on (ex: hero_of_dreams),
                    # which results in the estimate crawling up before it comes back down.
                    # Maybe a memory leak?
                    fps = result.fps
                else:
                    fps = replay.estimated_fps()
                frames_left = result.num_frames - result.frame
                durs.append(frames_left / fps)
                continue

            # Test is still starting up.
            if avg_fps_established:
                durs.append(replay.frames_limited() / avg_fps)
            else:
                durs.append(replay.estimated_duration())

        # For pending tests, divide the frames to be run by the running average FPS.
        # If average FPS has not been established yet, use the rough estimate duration.
        for replay in pending_replays:
            replay = replays_by_name[result.name]
            overhead = 10
            if avg_fps_established:
                durs.append(overhead + replay.frames_limited() / avg_fps)
            else:
                durs.append(overhead + replay.estimated_duration())

        s = compute_time(durs, concurrency)
        if s < 1:
            eta = 'ETA ~1s'
        elif s < 5:
            eta = 'ETA ~5s'
        elif s < 30:
            eta = 'ETA ~30s'
        elif s < 60:
            eta = 'ETA ~1m'
        else:
            eta = f'ETA {int(s / 60)}m'

        if avg_fps_established:
            return f'{eta} | {int(avg_fps)} fps'
        else:
            return eta

    def get_summary_msg(eta: str):
        num_failing = len([r for r in results if not r.success])
        return ' | '.join(
            [
                f'{len(results)}/{len(replays)}',
                f'{num_failing} failing' if num_failing else 'OK',
                eta,
            ]
        )

    while pending_replays or active_tests:
        while pending_replays and len(active_tests) < concurrency:
            replay = pending_replays.pop(0)
            test_index = replays.index(replay)
            run_dir = runs_dir / replay.path.with_suffix('').name
            run_dir.mkdir(parents=True)
            active_tests.append(_run_replay_test(ctx, test_index, replay, run_dir))

        next_active_tests = []
        active_results = []
        has_updated = False
        status = {}
        for active_test in active_tests:
            test_index, type, result = next(active_test, None)

            if type == 'status':
                has_updated = True
                next_active_tests.append(active_test)
                active_results.append(result)
                status[result.name] = 'status'
            elif type == 'finish':
                has_updated = True
                if _is_known_failure_test(result, ctx.arch):
                    result.success = True
                results.append(result)
                status[result.name] = 'finish'
        active_tests = next_active_tests

        if has_updated:
            eta = get_eta()
            summary = get_summary_msg(eta)
            on_update(
                RunReplayTestsProgress(
                    ctx.test_results,
                    results,
                    active_results,
                    pending_replays,
                    status,
                    eta,
                    summary,
                )
            )
        sleep(1)

    return [r for r in results if r]


def run_replays(
    replays: List[Replay],
    mode: str,
    runs_on: str,
    arch: str,
    test_results_dir: Path,
    build_folder: Path,
    extra_args: List[str],
    debugger=False,
    headless=True,
    jit=True,
    concurrency: Optional[Union[int, bool]] = True,
    prune_test_results=False,
    timeout=True,
    retries=0,
    on_update=Callable[[], RunReplayTestsProgress],
):
    test_results = ReplayTestResults(
        runs_on=runs_on,
        arch=arch,
        ci=is_ci,
        git_ref=os.environ.get('GITHUB_REF') if is_ci else None,
        workflow_run_id=os.environ.get('GITHUB_RUN_ID') if is_ci else None,
        zc_version='unknown',
        time=datetime.now(timezone.utc).isoformat(),
        runs=[],
    )
    if type(concurrency) == bool and concurrency:
        concurrency = os.cpu_count() - 4
    if concurrency < 1:
        concurrency = 1

    if test_results_dir.exists():
        shutil.rmtree(test_results_dir)
    test_results_dir.mkdir(parents=True)

    replays_remaining = replays

    print(f'running {len(replays)} replays\n')
    for i in range(retries + 1):
        if i != 0:
            last_runs = test_results.runs[-1]
            failures = [r.name for r in last_runs if not r.success]
            replays_remaining = [r for r in replays_remaining if r.name in failures]
        if not replays_remaining:
            break
        if i != 0:
            print('\nretrying failures...\n')

        runs_dir = test_results_dir / str(i)
        results = _run_replays(
            RunReplayTestsContext(
                test_results,
                mode,
                arch,
                replays_remaining,
                build_folder,
                test_results_dir,
                runs_dir,
                concurrency,
                timeout,
                debugger,
                headless,
                jit,
                extra_args,
            ),
            on_update,
        )
        test_results.runs.append(results)

        for result in results:
            run_dir = runs_dir / result.directory

            # Only print on failure and last attempt.
            if (not result.success or result.exceptions) and i == retries:
                print(f'failure: {result.name}')

                if result.exceptions:
                    print(f'  EXCEPTION: {" | ".join(result.exceptions)}')

                def print_nicely(title: str, path: Path):
                    if not path.exists():
                        return

                    title = f' {title} '
                    length = len(title) * 2
                    print()
                    print('=' * length)
                    print(title.center(length, '='))
                    print('=' * length)
                    print()
                    sys.stdout.buffer.write(path.read_bytes())

                print_nicely('STDOUT', run_dir / 'stdout.txt')
                print_nicely('STDERR', run_dir / 'stderr.txt')
                print_nicely('ALLEGRO LOG', run_dir / 'allegro.log')

    if prune_test_results:
        # Only keep the last run of each replay.
        replay_runs: List[RunResult] = []
        for runs in reversed(test_results.runs):
            for run in runs:
                if any(r for r in replay_runs if r.name == run.name):
                    continue
                replay_runs.append(run)

        for runs in test_results.runs:
            for run in runs:
                if run not in replay_runs:
                    shutil.rmtree(test_results_dir / run.directory)

        test_results.runs = [replay_runs]

        # These are huge and not necessary for the compare report.
        for file in test_results_dir.rglob('*.zplay.roundtrip'):
            file.unlink()

    (test_results_dir / 'test_results.json').write_text(test_results.to_json())
    return test_results


def _is_known_failure_test(run: RunResult, arch: str):
    if run.success:
        return False

    is_windows = platform.system() == 'Windows'
    name = Path(run.name).name
    ignore = False

    if (
        is_windows
        and name == 'the_deep_4_of_6.zplay'
        and run.unexpected_gfx_segments == [[40853, 40971]]
    ):
        ignore = True
    if arch == 'win32' and name == 'enigma_of_basilischi_island_basilse_1_of_2.zplay':
        if run.failing_frame == 135221:
            ignore = True
    if arch == 'win32' and name == 'enigma_of_basilischi_island_basilse_2_of_2.zplay':
        if run.failing_frame == 31839:
            ignore = True

    if ignore:
        print(f'!!! [{run.name}] filtering out known replay test failure !!!')
    return ignore
