#!/bin/bash

set -e

if ! command -v emcc &> /dev/null
then
  echo "emcc could not be found. You must install emsdk"
  echo "https://emscripten.org/docs/getting_started/downloads.html"
  exit 1
fi

EMCC_VERSION=3.1.9
emsdk install $EMCC_VERSION
emsdk activate $EMCC_VERSION

cd output/_auto
./buildpack.sh
rm -f buildpack/{zelda,zquest,zlauncher,zscript}
rm -rf buildpack/Addons buildpack/docs/ghost buildpack/docs/tango buildpack/utilities
rm -rf buildpack/headers/ghost_zh/3.0/demo buildpack/headers/GUITest.qst
rm -rf "buildpack/scripts/stdWeapons/example scripts"
rm buildpack/changelog.txt
rm buildpack/music/Isabelle_Z2.nsf
rm buildpack/zc.cfg && mv buildpack/zc_web.cfg buildpack/zc.cfg
find buildpack -name "*.dll" -type f -delete
find buildpack -name "*.so" -type f -delete
find buildpack -name "*.exe" -type f -delete
find buildpack -name "*.rtf" -type f -delete
find buildpack -name "*.pdf" -type f -delete
find buildpack -name "*.so" -type f -delete
find buildpack -name "*.zip" -type f -delete

LAZY_LOAD=(
  tilesets/classic.qst
)
mkdir -p buildpack_lazy
for f in ${LAZY_LOAD[@]}; do
  dir=$(dirname $f)
  mkdir -p "buildpack_lazy/$dir"
  mv "buildpack/$f" "buildpack_lazy/$dir"
done

rm -rf buildpack_zq
mkdir -p buildpack_zq
mkdir -p buildpack_zq/modules/classic/
mv buildpack/modules/classic/classic_qst.dat buildpack_zq/modules/classic/
cd -

mkdir -p build_emscripten
cd build_emscripten
if [[ "$DEBUG" ]]; then
  mkdir -p debug
  cd debug
else
  mkdir -p release
  cd release
fi

function build_js {
  ESBUILD_ARGS=()
  if ! [[ "$DEBUG" ]]; then
    ESBUILD_ARGS+=(
      --minify
    )
  fi
  npx esbuild --bundle ../../web/main.js --outfile=main.js --sourcemap ${ESBUILD_ARGS[@]}
  cp ../../web/styles.css .
}

# build_js && exit 0

# Wish I knew how to remove this.
EMCC_CACHE_DIR="$(dirname $(which emcc))/cache"
EMCC_CACHE_INCLUDE_DIR="$EMCC_CACHE_DIR/sysroot/include"
EMCC_CACHE_LIB_DIR="$EMCC_CACHE_DIR/sysroot/lib/wasm32-emscripten"

EMCC_FLAGS=(
  -s USE_FREETYPE=1
  -s USE_VORBIS=1
  -s USE_OGG=1
  -s USE_LIBJPEG=1
  -s USE_SDL=2
  -s USE_SDL_MIXER=2
  -s USE_LIBPNG=1
  -s USE_PTHREADS=1
  -I "$EMCC_CACHE_INCLUDE_DIR/AL"
)
LINKER_FLAGS=(
  --shell-file="../../web/index.html"
  --shared-memory
  -s EXPORTED_FUNCTIONS=_main,_create_synthetic_key_event,_get_shareable_url,_open_test_mode
  -s EXPORTED_RUNTIME_METHODS=cwrap
  -s FORCE_FILESYSTEM=1
  -s ASYNCIFY=1
  -s FULL_ES2=1
  -s SDL2_MIXER_FORMATS="['mid','mod','ogg','mp3','gme']"
  -s LLD_REPORT_UNDEFINED
  -s INITIAL_MEMORY=200MB
  -s ALLOW_MEMORY_GROWTH=1
  -s PTHREAD_POOL_SIZE=15
  # Necessary to avoid a deadlock. Bisected to here:
  # https://chromium.googlesource.com/external/github.com/emscripten-core/emscripten.git/+log/1a0b77c572ad..c48f73a5c763
  -s EXIT_RUNTIME=1
  -s MINIFY_HTML=0
  -lidbfs.js
  -lembind
)

CMAKE_BUILD_TYPE=""
if [[ "$DEBUG" ]]; then
  EMCC_FLAGS+=(
    -O2
    -g
  )
  LINKER_FLAGS+=(
    # --threadprofiler
    -s ASSERTIONS=1
  )
  # CMAKE_BUILD_TYPE="Debug"
  CMAKE_BUILD_TYPE="Release"
else
  EMCC_FLAGS+=(
    -O3
  )
  CMAKE_BUILD_TYPE="Release"
fi

# Find memory leaks.
# EMCC_FLAGS+=(-fsanitize=leak)
# LINKER_FLAGS+=(-fsanitize=leak -s EXIT_RUNTIME)

# LINKER_FLAGS+=(-s SAFE_HEAP=1)
# EMCC_FLAGS+=(--memoryprofiler)

emcmake cmake ../.. \
  -D CMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE \
  -D ALLEGRO_SDL=ON \
  -D WANT_ALLOW_SSE=OFF \
  -D WANT_OPENAL=OFF \
  -D WANT_ALSA=OFF \
  -D SDL2_INCLUDE_DIR="$EMCC_CACHE_INCLUDE_DIR" \
  -D SDL2_LIBRARY="$EMCC_CACHE_LIB_DIR/libSDL2-mt.a" \
  -D CMAKE_C_FLAGS="${EMCC_FLAGS[*]}" \
  -D CMAKE_CXX_FLAGS="${EMCC_FLAGS[*]} -D_NPASS" \
  -D CMAKE_EXE_LINKER_FLAGS="${LINKER_FLAGS[*]}" \
  -D CMAKE_EXECUTABLE_SUFFIX_CXX=".html"

sh ../../patches/apply.sh

TARGETS="${@:-zelda zquest}"
cmake --build . -t $TARGETS

"$(dirname $(which emcc))"/tools/file_packager.py zc.data \
  --no-node \
  --preload "../../output/_auto/buildpack@/" \
  --preload "../../timidity/zc.cfg@/etc/timidity.cfg" \
  --use-preload-cache \
  --js-output=zc.data.js

# Zquest also uses zc.data
"$(dirname $(which emcc))"/tools/file_packager.py zq.data \
  --no-node \
  --preload "../../output/_auto/buildpack_zq@/" \
  --use-preload-cache \
  --js-output=zq.data.js

# https://github.com/emscripten-core/emscripten/issues/11952
function fix_hash {
  HASH=$(shasum -a 256 $1 | awk '{print $1}')
  sed -i -e "s/\"package_uuid\": \"[^\"]*\"/\"package_uuid\":\"$HASH\"/" "$2"
  if ! grep -q "$HASH" "$2"
  then
    echo "failed to replace data hash"
    exit 1
  fi
}

fix_hash zc.data zc.data.js
fix_hash zq.data zq.data.js

function set_files {
  R=$(jq --compact-output --null-input '$ARGS.positional' --args "${LAZY_LOAD[@]}")
  sed -i -e "s|files: \[\]|files: $R|" $1
}

function insert_css {
  sed " /*__INLINECSS__*/  r ../../web/styles.css" "$1" > tmp.html
  mv tmp.html "$1"
}

if [ -f zelda.html ]; then
  sed -i -e 's/__TARGET__/zelda/' zelda.html
  sed -i -e 's|__DATA__|<script src="zc.data.js"></script>|' zelda.html
  set_files zelda.html
  insert_css zelda.html
fi
if [ -f zquest.html ]; then
  sed -i -e 's/__TARGET__/zquest/' zquest.html
  sed -i -e 's|__DATA__|<script src="zc.data.js"></script><script src="zq.data.js"></script>|' zquest.html
  set_files zquest.html
  insert_css zquest.html
fi

cp -r ../../timidity .

rm -rf files
mv ../../output/_auto/buildpack_lazy files

build_js

# Now start a local webserver in the build_emscripten folder:
#   npx statikk --port 8000 --coi
