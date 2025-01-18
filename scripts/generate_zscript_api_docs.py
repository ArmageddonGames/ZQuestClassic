from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
import sys
import textwrap
import time

from dataclasses import dataclass
from enum import Enum, auto
from pathlib import Path
from typing import Optional

script_dir = Path(os.path.dirname(os.path.realpath(__file__)))
root_dir = script_dir.parent
resources_dir = root_dir / 'resources'
sources_dir = root_dir / 'docs-www/source'
tutorials_dir = root_dir / 'docs-www/source/tutorials'
zscript_dir = root_dir / 'docs-www/source/zscript'

sys.path.append(str((root_dir / 'scripts').absolute()))
import run_target

libraries = ['std', 'ghost', 'tango', 'gui', 'EmilyMisc']
sections = {
    'Globals': [],
    'Classes': [],
    'Libraries': [],
}
git_ref = subprocess.check_output(['git', 'rev-parse', 'HEAD'], encoding='utf-8').strip()


def indent(text: str, amount: int, ch=' ') -> str:
    return textwrap.indent(text, amount * ch)


class SymbolKind(Enum):
    File = 1
    Module = auto()
    Namespace = auto()
    Package = auto()
    Class = auto()
    Method = auto()
    Property = auto()
    Field = auto()
    Constructor = auto()
    Enum = auto()
    Interface = auto()
    Function = auto()
    Variable = auto()
    Constant = auto()
    String = auto()
    Number = auto()
    Boolean = auto()
    Array = auto()
    Object = auto()
    Key = auto()
    Null = auto()
    EnumMember = auto()
    Struct = auto()
    Event = auto()
    Operator = auto()
    TypeParameter = auto()


seen_warnings = []


def warning(text: str):
    if text not in seen_warnings:
        print(text)
        seen_warnings.append(text)


@dataclass
class Location:
    ref: str
    file: str
    line: int

    def reflink(self):
        url = f'https://github.com/ZQuestClassic/ZQuestClassic/blob/{git_ref}/{self.file}#L{self.line}'
        return f'`Source <{url}>`__'


@dataclass
class Comment:
    text: str
    tags: dict[str, str]


@dataclass
class Type:
    name: str
    const: bool

    def is_array(self) -> bool:
        return '[]' in self.name

    def __str__(self):
        const_prefix = '|const| ' if self.const else ''
        id = self.name.replace('[]', '')
        if id in ['void', 'int', 'long', 'untyped', 'T', 'T1', 'T2']:
            return f'{const_prefix}|{id}|\\ {self.name.replace(id, "")}'
        if id in ['bool', 'char32', 'rgb']:
            return f'{const_prefix}{self.name}'

        symbol = types.get(id)
        if symbol:
            return f'{const_prefix}{reflink(symbol, self.name)}'

        warning(f'TODO: cant link {self.name}')
        return f'{const_prefix}{self.name}'


@dataclass
class Variable:
    symbol_id: int
    loc: Location
    comment: Comment
    name: str
    type: Type
    value: Optional[str]

    def link(self) -> str:
        return reflink(self, self.name)

    def deprecated(self) -> bool:
        return self.comment and bool(self.comment.tags.get('deprecated'))


@dataclass
class EnumMember:
    symbol_id: int
    loc: Location
    comment: Comment
    name: str
    value: str
    type: Type


@dataclass
class Enum:
    symbol_id: int
    loc: Location
    comment: Comment
    name: str
    members: list[EnumMember]


@dataclass
class Parameters:
    variables: list[Variable]
    varargs: Optional[int]

    def __str__(self) -> str:
        parts = []
        for var in self.variables:
            def_str = ''
            if var.value != None:
                value_str = format_value(var.type, var.value)
                def_str = f' = ``{value_str}``'
            parts.append(f'{var.type} {var.name}{def_str}')
        if self.varargs != None:
            if self.variables and self.variables[-1].type.name == 'T':
                parts.append(f'{self.variables[-1].type} |varargs| ...')
            else:
                parts.append('untyped |varargs| ...')
        text = ', '.join(parts)
        return f'({text})'


@dataclass
class Function:
    symbol_id: int
    loc: Location
    comment: Comment
    name: str
    parameters: Parameters
    return_type: Type
    constructor: bool

    def link(self) -> str:
        return reflink(self, self.name)

    def deprecated(self) -> bool:
        return self.comment and bool(self.comment.tags.get('deprecated'))


@dataclass
class Class:
    symbol_id: int
    loc: Location
    comment: Comment
    name: str
    parent: Optional[Class]
    constructors: list[Function]
    functions: list[Function]
    variables: list[Variable]
    enums: list[Enum]

    def is_global(self) -> bool:
        return self.name[0].isupper()


@dataclass
class File:
    name: str
    enums: list[Enum]
    classes: list[Class]
    functions: list[Function]
    variables: list[Variable]


@dataclass
class Scope:
    name: str
    enums: list[Enum]
    classes: list[Class]
    functions: list[Function]
    variables: list[Variable]


def format_value(type: Type, value) -> str:
    if isinstance(value, str):
        return value
    if type.is_array() and value == 0:
        return 'null'
    if type.name == 'bool':
        return 'true' if value else 'false'
    if type.name == 'int' or type.name == 'char32' or type.name == 'rgb':
        return f'{value / 10000:g}'
    if type.name == 'long':
        return f'{value}L'
    return value


def parse_doc_type(type_str: str) -> Type:
    const = False
    if type_str.startswith('const '):
        type_str = type_str[6:]
        const = True
    if type_str == 'screendata':
        type_str = 'Screen'
    if type_str == 'hero':
        type_str = 'Hero'
    return Type(name=type_str, const=const)


def parse_doc_comment(x) -> Optional[Comment]:
    if x.get('comment'):
        text = x['comment']['text']
        tags = x['comment']['tags']
        return Comment(text=text, tags=tags)

    return None


def parse_doc_parameters(x) -> Parameters:
    variables = []
    varargs = x.get('varargs', None)

    params = x['parameters']
    for i, param in enumerate(params):
        name = param['name']
        type = parse_doc_type(param['type'])
        value = param.get('default', None)
        variables.append(
            Variable(
                symbol_id=None,
                loc=None,
                comment=None,
                name=name,
                type=type,
                value=value,
            )
        )

    return Parameters(variables=variables, varargs=varargs)


def parse_doc_enum_member(x) -> EnumMember:
    symbol_id = x['id']
    loc = parse_location(x)
    comment = parse_doc_comment(x)
    name = x['name']
    value = x['value']
    type = parse_doc_type(x['type'])
    return EnumMember(
        symbol_id=symbol_id, loc=loc, comment=comment, name=name, value=value, type=type
    )


def parse_location(x) -> Location:
    file = Path(x['location']['file'])
    file = file.relative_to(root_dir).as_posix()
    return Location(ref='', file=file, line=x['location']['line'])


def parse_doc_symbol(x, parent=None) -> File:
    kind = SymbolKind(x['kind'])
    if kind == SymbolKind.Variable:
        symbol_id = x['id']
        loc = parse_location(x)
        comment = parse_doc_comment(x)
        name = x['name']
        type = parse_doc_type(x['type'])
        value = x.get('value', None)
        return Variable(
            symbol_id=symbol_id,
            loc=loc,
            comment=comment,
            name=name,
            type=type,
            value=value,
        )
    elif kind == SymbolKind.Enum:
        symbol_id = x['id']
        loc = parse_location(x)
        comment = parse_doc_comment(x)
        name = x['name']
        members = [parse_doc_enum_member(m) for m in x['children']]

        # For now, give unnamed enums a fake name.
        # TODO: Eventually, let's just update enums to have names.
        if not name:
            if members:
                name = next((x for x in members[0].name.split('_') if x), 'UnknownEnum')
            else:
                name = 'UnknownEnum'

        return Enum(
            symbol_id=symbol_id, loc=loc, comment=comment, name=name, members=members
        )
    elif kind in [SymbolKind.Function, SymbolKind.Constructor]:
        symbol_id = x['id']
        loc = parse_location(x)
        comment = parse_doc_comment(x)
        name = x['name']
        return_type = parse_doc_type(x['returnType'])
        parameters = parse_doc_parameters(x)
        return Function(
            symbol_id=symbol_id,
            loc=loc,
            comment=comment,
            name=name,
            parameters=parameters,
            return_type=return_type,
            constructor=kind == SymbolKind.Constructor,
        )
    elif kind == SymbolKind.Class:
        symbol_id = x['id']
        loc = parse_location(x)
        comment = parse_doc_comment(x)
        name = x['name']
        if name == 'screendata':
            name = 'Screen'
        if name == 'hero':
            name = 'Hero'
        parent = x.get('parent', None)  # 'str' for now, but set to Class later.
        constructors = []
        functions = []
        variables = []
        enums = []
        for symbol in x['children']:
            kind = SymbolKind(symbol['kind'])
            if kind == SymbolKind.Function:
                functions.append(parse_doc_symbol(symbol, name))
            elif kind == SymbolKind.Constructor:
                constructors.append(parse_doc_symbol(symbol, name))
            elif kind == SymbolKind.Variable:
                variables.append(parse_doc_symbol(symbol, name))
            elif kind == SymbolKind.Enum:
                enums.append(parse_doc_symbol(symbol, name))

        return Class(
            symbol_id=symbol_id,
            loc=loc,
            comment=comment,
            name=name,
            parent=parent,
            constructors=constructors,
            functions=functions,
            variables=variables,
            enums=enums,
        )


def parse_doc_file(x) -> File:
    file = File(name=x['name'], enums=[], classes=[], functions=[], variables=[])
    for symbol in x['symbols']:
        kind = SymbolKind(symbol['kind'])
        if kind == SymbolKind.Variable:
            file.variables.append(parse_doc_symbol(symbol))
        elif kind == SymbolKind.Enum:
            file.enums.append(parse_doc_symbol(symbol))
        elif kind == SymbolKind.Function:
            file.functions.append(parse_doc_symbol(symbol))
        elif kind == SymbolKind.Class:
            file.classes.append(parse_doc_symbol(symbol))
    return file


def get_doc_data(script_path) -> list[File]:
    tmp_file = root_dir / '.tmp/tmp.zs'
    tmp_file.write_text(
        '\n'.join(
            [
                '#include "std.zh"',
                '#include "ghost.zh"',
                '#include "tango.zh"',
                '#include "EmilyMisc.zh"',
                '#include "gui.zh"',
            ]
        )
    )

    # Change include paths to use resources/ directly, instead of possibly-stale stuff inside a build folder.
    include_paths = [
        str(root_dir / 'resources/include'),
        str(root_dir / 'resources/headers'),
    ]
    args = [
        '-input',
        str(tmp_file),
        '-include',
        ';'.join(include_paths),
        '-unlinked',
        '-delay_cassert',
        '-doc',
    ]
    p = run_target.run('zscript', args)
    stdout = p.stdout.replace(str(script_path), script_path.name)
    if p.returncode:
        raise Exception(p.stderr)

    data = json.loads(stdout)
    # print(json.dumps(data,indent=2))
    # exit(0)
    files = []
    for file in data['files']:
        file = parse_doc_file(file)
        if file.variables or file.functions or file.classes or file.enums:
            files.append(file)
    files.sort(key=lambda x: x.name)

    # Set Class.parent
    classes = {}
    for file in files:

        def get_classes(symbol, parent):
            if isinstance(symbol, Class):
                classes[symbol.name] = symbol

        walk(file, get_classes)
    for symbol in classes.values():
        if symbol.parent:
            symbol.parent = classes[symbol.parent]

    return files


def walk(symbol, cb, parent=None):
    cb(symbol, parent)
    if isinstance(symbol, Class):
        for x in symbol.functions:
            walk(x, cb, symbol)
        for x in symbol.constructors:
            walk(x, cb, symbol)
        for x in symbol.variables:
            walk(x, cb, symbol)
    elif isinstance(symbol, File) or isinstance(symbol, Scope):
        for x in symbol.functions:
            walk(x, cb, symbol)
        for x in symbol.variables:
            walk(x, cb, symbol)
        for x in symbol.classes:
            walk(x, cb, symbol)
        for x in symbol.enums:
            walk(x, cb, symbol)
    elif isinstance(symbol, Enum):
        for x in symbol.members:
            walk(x, cb, symbol)


def get_ref(symbol) -> str:
    pass


all_files = get_doc_data(root_dir / 'tests/scripts/std.zs')
lines = []
types = {}
symbols_by_id = {}

# Create refs.
seen_refs = []
for file in all_files:

    def cb(symbol, parent):
        if hasattr(symbol, 'symbol_id'):
            if symbol.symbol_id:
                symbols_by_id[symbol.symbol_id] = symbol

        if isinstance(symbol, Class) or isinstance(symbol, Enum):
            types[symbol.name] = symbol

        if not hasattr(symbol, 'loc'):
            return

        if 'bindings' in file.name:
            ref = 'globals_'
            if isinstance(parent, Class):
                ref = 'globals_' if parent.is_global() else 'classes_'
            elif isinstance(symbol, Class):
                ref = 'globals_' if symbol.is_global() else 'classes_'
        else:
            rel_file = Path(file.name).relative_to(resources_dir)
            lib = next((x for x in libraries if x in str(rel_file)), None)
            if lib:
                ref = f'libs_{lib}_'
            else:
                raise Exception(f'unknown file: {file.name}')

        if 'bindings' in file.name and isinstance(parent, Class):
            ref += f'{parent.name}_'

        if isinstance(symbol, Function):
            if symbol.constructor:
                ref += 'ctor_'
            else:
                ref += 'fun_'
        elif isinstance(symbol, Variable):
            ref += 'var_'
        elif isinstance(symbol, Enum):
            ref += 'enum_'
        elif isinstance(symbol, EnumMember):
            ref += 'enum_member_'
        ref += symbol.name

        # Sphinx refs are normalized to lowercase.
        ref = ref.lower()

        # Needed for function overloading.
        initial_ref = ref
        attempts = 1
        while ref in seen_refs:
            attempts += 1
            ref = f'{initial_ref}_{attempts}'

        symbol.loc.ref = ref
        seen_refs.append(ref)

    walk(file, cb)


def rst_title(text: str):
    lines.append(text)
    lines.append('=' * len(text))
    lines.append('')


def rst_h2(text: str):
    lines.append(text)
    lines.append('-' * len(text))
    lines.append('')


def reflink(symbol, label='🔗') -> str:
    return f':ref:`{label}<{symbol.loc.ref}>`'


def add(text: str):
    lines.append(text)


def add_table(rows: list[list[str]]):
    if not rows:
        return

    for i, row in enumerate(rows):
        rows[i] = [f' {v} ' for v in row]

    cols = len(rows[0])
    col_widths = []
    for i in range(cols):
        w = 0
        for row in rows:
            w = max(w, len(row[i]))
        col_widths.append(w)
    table_width = sum(col_widths)

    add('.. table::')
    add('   :widths: auto')
    add('')

    divider = '+'
    for x in col_widths:
        divider += '-' * x + '+'

    add('   ' + divider)
    for row in rows:
        cols = []
        for i, v in enumerate(row):
            cols.append(v.ljust(col_widths[i]))
        add('   ' + '|' + '|'.join(cols) + '|')
        add('   ' + divider)

    add('')


def rst_toc(caption: str, documents):
    add('.. toctree::')
    add('   :maxdepth: 1')
    add('   :class: toctree-grid')
    if caption:
        add(f'   :caption: {caption}')
    add('')
    for document in documents:

        add(f'   /{document}')
    add('')


def add_comment(symbol):
    def replace_symbol_link(match: re.Match):
        if '|' in match.group(1):
            symbol_id, label = match.group(1).split('|')
            matched_symbol = symbols_by_id.get(int(symbol_id))
        else:
            matched_symbol = symbols_by_id.get(int(match.group(1)))
            label = matched_symbol.name

        if not matched_symbol:
            raise Exception(
                f'could not resolve symbol link in comment: {symbol.comment.text}'
            )

        return reflink(matched_symbol, label)

    monos = []

    def replace_monos(match: re.Match):
        monos.append(match.group(1))
        return '$MONO'

    def replace_monos_placeholder(match: re.Match):
        return f'``{monos.pop(0)}``'

    code_blocks = []

    def replace_code_block(match: re.Match):
        code = match.group(1).replace('\n', '\n   ')
        code_blocks.append(code)
        return '\n.. zscript::\n   $CODE'

    def replace_code_block_placeholder(match: re.Match):
        return code_blocks.pop(0)

    def sanitize(text: str) -> str:
        text = re.sub(r'```(.+?)```', replace_code_block, text, flags=re.DOTALL)
        text = re.sub(r'`(.+?)`', replace_monos, text)
        text = text.replace('_', '\\_').replace('*', '\\*').replace('`', '``')
        text = re.sub(r'\$CODE', replace_code_block_placeholder, text)
        text = re.sub(r'\$MONO', replace_monos_placeholder, text)
        text = re.sub(r'\[@(.+?)@\]', replace_symbol_link, text)
        return text

    if symbol.comment and symbol.comment.text:
        add('')
        add('.. rst-class:: classref-comment')
        add('')
        for tag, value in symbol.comment.tags.items():
            if tag in ['value', 'index', 'param', 'length']:
                for line in value.splitlines():
                    add(indent(sanitize(f'`{tag}` ' + line), 3))
                    add('')
                    add('')
        add('')
        add(indent(sanitize(symbol.comment.text), 3))
        add('')

    if symbol.comment and symbol.comment.tags.get('deprecated'):
        notice = symbol.comment.tags['deprecated']
        if notice == True:
            notice = ''
        else:
            notice = notice.replace('\n', ' ')
        add('')
        add(f'.. warning:: **Deprecated!** {sanitize(notice)}')
        add('')


def handle_variable(symbol: Variable):
    add(f'.. _{symbol.loc.ref}:\n')

    parts = [
        str(symbol.type),
        f'**{symbol.name}**',
        (
            f'= ``{format_value(symbol.type, symbol.value)}``'
            if symbol.value != None
            else ''
        ),
        reflink(symbol),
    ]
    if symbol.loc:
        parts.append(symbol.loc.reflink())
    add(' '.join(parts))
    add_comment(symbol)

    add('')
    add('.. rst-class:: classref-item-separator')
    add('')
    add('----')
    add('')


def handle_enum(symbol: Enum):
    add(f'.. _{symbol.loc.ref}:\n')

    add(f'enum **{symbol.name}**: {reflink(symbol)}')
    if symbol.loc:
        add(symbol.loc.reflink())
    add('')
    add_comment(symbol)

    for member in symbol.members:
        add(f'.. _{member.loc.ref}:')
        add('')
        add('.. rst-class:: classref-enumeration-constant')
        add('')
        value_str = format_value(member.type, member.value)
        add(f'{reflink(symbol, symbol.name)} **{member.name}** = ``{value_str}``')
        add_comment(member)
        add('')

    add('')
    add('.. rst-class:: classref-item-separator')
    add('')
    add('----')
    add('')


def handle_function(symbol: Function):
    add(f'.. _{symbol.loc.ref}:\n')
    add('.. rst-class:: classref-method\n')

    parts = [
        str(symbol.return_type),
        f'**{symbol.name}**' + '\\ ' + str(symbol.parameters),
        reflink(symbol),
    ]
    if symbol.loc:
        parts.append(symbol.loc.reflink())
    add(' '.join(parts))
    add_comment(symbol)

    add('')
    add('.. rst-class:: classref-item-separator')
    add('')
    add('----')
    add('')


def handle_scope(symbol):
    constructors = []
    functions = []
    variables = []
    enums = []

    def cb(symbol, parent):
        if isinstance(symbol, Function):
            if symbol.constructor:
                constructors.append(symbol)
            else:
                functions.append(symbol)
        elif isinstance(symbol, Variable):
            variables.append(symbol)
        elif isinstance(symbol, Enum):
            enums.append(symbol)

    walk(symbol, cb)

    def deprecated(x):
        return ' **deprecated**' if x.deprecated() else ''

    if variables:
        rst_h2('Variables')
        add_table([[str(x.type), x.link() + deprecated(x)] for x in variables])

    if constructors:
        rst_h2('Constructors')
        add_table(
            [
                [
                    str(x.return_type),
                    (x.link() + '\\ ' + str(x.parameters) + deprecated(x)),
                ]
                for x in constructors
            ]
        )

    if functions:
        rst_h2('Functions')
        add_table(
            [
                [
                    str(x.return_type),
                    (x.link() + '\\ ' + str(x.parameters) + deprecated(x)),
                ]
                for x in functions
            ]
        )

    if enums:
        rst_h2('Enumerations')
        for enum in enums:
            handle_enum(enum)

    if variables:
        rst_h2('Variable descriptions')
        for variable in variables:
            handle_variable(variable)

    if constructors:
        rst_h2('Constructor descriptions')
        for constructor in constructors:
            handle_function(constructor)

    if functions:
        rst_h2('Function descriptions')
        for function in functions:
            handle_function(function)

    # Remove the last five lines (separator stuff), since there is nothing that follows.
    global lines
    if lines[-1] == '' and lines[-2] == '----':
        lines = lines[0:-4]


def process_bindings_class(file: File, symbol: Class):
    add(f'.. _{symbol.loc.ref}:\n')
    rst_title(symbol.name)

    if symbol.parent:
        parents = []
        cur = symbol.parent
        while cur:
            parents.append(cur)
            cur = cur.parent
        parts = [reflink(s, s.name) for s in parents]
        add(f'Inherits: {" < ".join(parts)}')
        add('')

    inherited_by = []
    for s in types.values():
        if isinstance(s, Class) and s.parent == symbol:
            inherited_by.append(s)
    if inherited_by:
        inherited_by.sort(key=lambda s: s.name)
        parts = [reflink(s, s.name) for s in inherited_by]
        add(f'Inherited by: {", ".join(parts)}')
        add('')

    add_comment(symbol)

    if symbol.comment and symbol.comment.tags.get('tutorial'):
        tutorial = symbol.comment.tags.get('tutorial')

        rst_h2('Tutorials')
        label = tutorial.replace('tutorials/', '').capitalize()
        # ref = tutorial.replace('/', '_')
        add(f'* :doc:`{tutorial}`')
        add('')

    handle_scope(
        Scope(
            name='',
            enums=file.enums,
            classes=[],
            functions=[*symbol.constructors, *symbol.functions],
            variables=symbol.variables,
        )
    )


def process_lib(name: str, files):
    lib_documents = []

    for file in files:
        path = Path(file.name)
        fname = path.stem
        if path.name == 'std.cfg':
            fname = 'cfg'
        rst_title(path.name)

        handle_scope(file)

        doc = f'zscript/libs/{name}/{fname}'
        lib_documents.append(doc)
        write(doc)

    lib_documents.sort()

    document = f'zscript/libs/{name}/index'
    sections['Libraries'].append(document)
    rst_title(lib)
    rst_toc(None, lib_documents)

    example = None
    docs = None
    if name == 'gui':
        example = 'gui_example.zs'
    elif name == 'tango':
        example = 'tango_example.zs'
        docs = 'The original documentation for tango can be found `here <../../../_static/old/tango/index.html>`__'
    elif name == 'ghost':
        docs = 'The original documentation for ghost can be found `here <../../../_static/old/ghost/index.html>`__'

    if docs:
        rst_h2('Docs')
        add(docs)
        add('')

    if example:
        rst_h2('Example')
        add('.. only:: html')
        add('')
        url = f'https://raw.githubusercontent.com/ZQuestClassic/ZQuestClassic/refs/heads/{git_ref}/resources/headers/examples/{example}'
        add('   .. zscript::')
        add(f'      :url: {url}')
        add(f'      :fname: {example}')
        add('')

        add('.. only:: not html')
        add('')
        add(f'   .. literalinclude :: /../../resources/headers/examples/{example}')
        add('')

    write(document)


def write(name: str):
    global lines

    out_file = sources_dir / f'{name}.rst'
    out_file.parent.mkdir(parents=True, exist_ok=True)
    text = '.. Auto-generated by generate_zscript_api_docs.py\n\n' + '\n'.join(lines)
    out_file.write_text(text, 'utf-8')
    lines = []


if zscript_dir.exists():
    shutil.rmtree(zscript_dir)
zscript_dir.mkdir(parents=True)

# handle bindings
classes = []
functions = []
variables = []
enums = []
for file in [f for f in all_files if 'bindings' in f.name]:
    if not file.classes:
        functions.extend(file.functions)
        variables.extend(file.variables)
        enums.extend(file.enums)
        continue

    if len(file.classes) != 1:
        raise Exception('dunno how to handle multiple classes here')

    # Put the functions/variables into the global documentation page, but
    # keep the enums (see process_bindings_class).
    functions.extend(file.functions)
    variables.extend(file.variables)

    symbol = file.classes[0]
    is_global = symbol.is_global()
    section = sections['Globals'] if is_global else sections['Classes']
    prefix = 'globals' if is_global else 'classes'
    document = f'zscript/{prefix}/{symbol.name}'
    section.append(document)
    process_bindings_class(file, symbol)
    write(document)

document = 'zscript/globals/constants'
sections['Globals'].insert(0, document)
rst_title('Global constants')
handle_scope(
    Scope(name='constants', enums=enums, classes=[], functions=[], variables=variables)
)
write(document)

document = 'zscript/globals/functions'
sections['Globals'].insert(0, document)
rst_title('Global functions')
handle_scope(
    Scope(name='globals', enums=[], classes=[], functions=functions, variables=[])
)
write(document)

# handle libs
for lib in libraries:
    files = [f for f in all_files if lib in f.name]
    process_lib(lib, files)

# zscript/index.rst
rst_title('ZScript')
for name, documents in sections.items():
    if name == 'Globals':
        rst_toc(name, documents[0:2])
        rst_toc(None, documents[2:])
        continue

    rst_toc(name, documents)
write('zscript/index')
