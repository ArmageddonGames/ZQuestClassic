import html
import urllib

from docutils import nodes
from docutils.parsers.rst import Directive
from docutils.parsers.rst.directives import choice, flag, nonnegative_int, unchanged, unchanged_required
from docutils.parsers.rst.directives.admonitions import BaseAdmonition
from sphinx import addnodes
from sphinx.application import Sphinx
from sphinx.locale import _
from sphinx.util.docutils import SphinxDirective, SphinxTranslator
from sphinx.util.typing import ExtensionMetadata

def code_style_options(arg: str):
    return choice(arg, ('plain','file','body'))
def list_options(arg: str):
    return arg.split()
def nonnegative_int_required(arg: str):
    if arg is None:
        raise ValueError('argument required but none supplied')
    return nonnegative_int(arg)

class ZScriptNode(nodes.General, nodes.Element):
    url: str = ''
    data: str = ''
    fname: str = ''
    height: int = 800
    style: str = 'file'
    inline: bool = False
    def __init__(self, url: str = '', data: str = '', fname: str = '',
        height: int = 800, style: str = 'file', inline: bool = False):
        super().__init__()
        self.url = url
        self.data = data
        self.fname = fname
        self.height = height
        self.style = style
        self.inline = inline


class ZScriptDirective(SphinxDirective):
    has_content: bool = True
    option_spec: dict = {
        'url': unchanged,
        'fname': unchanged,
        'height': int,
        'style': code_style_options,
        'inline': flag,
    }

    def run(self) -> list[nodes.Node]:
        return [ZScriptNode(
            url=self.options.get('url', '').strip(),
            data='\n'.join(self.content),
            fname=self.options.get('fname', '').strip(),
            height=self.options.get('height', 800),
            style=self.options.get('style', 'file'),
            inline='inline' in self.options,
            )]


body_to_cls = {
    'plain': 'nohighlight',
    'body': 'language-zs-body',
    'file': 'language-zs',
}
def visit_zscript_node_html(translator: SphinxTranslator, node: ZScriptNode) -> None:
    if node.data:
        cls = body_to_cls.get(node.style, 'language-zs')
        all_classes = ' '.join(['hljs', cls] + node.get('classes', []))
        if node.inline:
            html_str = f'<code class="inlinezs pre literal {all_classes}">{html.escape(node.data)}</code>'
        else:
            html_str = f'<pre class="{all_classes}"><code>{html.escape(node.data)}</code></pre>'
        html_str += f'''
        <script>
            {{
                const el = document.currentScript.previousElementSibling;
                requestAnimationFrame(() => hljs.highlightElement(el));
            }}
        </script>
        '''
        translator.body.append(html_str)
    else:
        html_str = f'<iframe width=100%% height={node.height} src=https://web.zquestclassic.com/zscript/'
        first = False
        if node.url != '':
            html_str += '?url=' + node.url
        elif node.data != '':
            html_str += '?data=' + urllib.parse.quote(node.data)
        else: first = True
        if node.fname != '':
            html_str += '?' if first else '&'
            html_str += 'fname=' + urllib.parse.quote(node.fname)
        html_str += '></iframe>\n'
        translator.body.append(html_str)

def visit_zscript_node_unsupported(translator: SphinxTranslator, node: ZScriptNode) -> None:
    logger.warning("zscript: unsupported output format (node skipped)")
    raise nodes.SkipNode

class AdmonitionNode(nodes.Admonition, nodes.Element):
    pass

class TodoNode(AdmonitionNode):
    pass

class TodoDirective(BaseAdmonition, SphinxDirective):
    node_class = TodoNode
    optional_arguments: int = 1
    final_argument_whitespace: bool = True
    option_spec: dict = {
        'nowarn': flag
    }
    
    def run(self) -> list[nodes.Node]:
        self.options['class'] = ['admonition-todo']
        
        (node,) = super().run()
        title = 'TODO'
        if len(self.arguments) and len(self.arguments[0]):
            title += f': {self.arguments[0]}'
        node.insert(0, nodes.title(text=_(title)))
        node['docname'] = self.env.docname
        self.add_name(node)
        self.set_source_info(node)
        self.state.document.note_explicit_target(node)
        
        if not 'nowarn' in self.options:
            self.reporter.warning(f'{title}: {"\n".join(self.content)}')
        return [node]

class PlansNode(AdmonitionNode):
    pass

class PlansDirective(BaseAdmonition, SphinxDirective):
    node_class = PlansNode
    optional_arguments: int = 1
    final_argument_whitespace: bool = True
    
    def run(self) -> list[nodes.Node]:
        self.options['class'] = ['admonition-plans']
        
        (node,) = super().run()
        title = 'Not Yet Implemented'
        if len(self.arguments) and len(self.arguments[0]):
            title = f': {self.arguments[0]}'
        node.insert(0, nodes.title(text=_(title)))
        node['docname'] = self.env.docname
        self.add_name(node)
        self.set_source_info(node)
        self.state.document.note_explicit_target(node)
        
        return [node]

def visit_admonition_node_html(translator: SphinxTranslator, node: AdmonitionNode) -> None:
    translator.visit_admonition(node)
def depart_admonition_node_html(translator: SphinxTranslator, node: AdmonitionNode) -> None:
    translator.depart_admonition(node)


class ZSDeprecated(SphinxDirective):
    """
    Directive to describe a deprecation in a specific version.
    Adapted from 'sphinx.domains.changeset's 'VersionChange'
    """

    has_content = True
    required_arguments = 0
    optional_arguments = 1
    final_argument_whitespace = True

    def run(self) -> list[nodes.Node]:
        node = addnodes.versionmodified()
        node.document = self.state.document
        self.set_source_info(node)
        node['type'] = 'deprecated'
        has_version = len(self.arguments) > 0
        if has_version:
            node['version'] = self.arguments[0]
            text = f'Deprecated since version {self.arguments[0]}'
        else:
            text = 'Deprecated'
        if self.content:
            node += self.parse_content_to_nodes()
        classes = ['versionmodified', 'deprecated']
        if len(node) > 0 and isinstance(node[0], nodes.paragraph):
            # the contents start with a paragraph
            if node[0].rawsource:
                # make the first paragraph translatable
                content = nodes.inline(node[0].rawsource, translatable=True)
                content.source = node[0].source
                content.line = node[0].line
                content += node[0].children
                node[0].replace_self(
                    nodes.paragraph('', '', content, translatable=False)
                )

            para = node[0]
            para.insert(0, nodes.inline('', '%s: ' % text, classes=classes))
        elif len(node) > 0:
            # the contents do not starts with a paragraph
            para = nodes.paragraph(
                '',
                '',
                nodes.inline('', '%s: ' % text, classes=classes),
                translatable=False,
            )
            node.insert(0, para)
        else:
            # the contents are empty
            para = nodes.paragraph(
                '',
                '',
                nodes.inline('', '%s.' % text, classes=classes),
                translatable=False,
            )
            node.append(para)
        
        if has_version:
            domain = self.env.domains.changeset_domain
            domain.note_changeset(node)

        return [node]


class StylizeNode(nodes.General, nodes.Element):
    classes: list[str] = []
    def __init__(self, classes: list = []):
        super().__init__()
        self.classes = classes

class Stylize(SphinxDirective):
    optional_arguments: int = 1
    final_argument_whitespace: bool = True
    option_spec: dict = {
        'classes': list_options
    }
    def run(self) -> list[nodes.Node]:
        c = self.options.get('classes', [])
        if len(self.arguments):
            c.extend(self.arguments[0].split())
        return [StylizeNode(classes = c)]
def visit_stylize_node_html(translator: SphinxTranslator, node: StylizeNode) -> None:
    if node.classes == []:
        raise ValueError('.. style:: cannot exist with no style specified!')
    html_str = f'''
    <script>
        {{
            const scr = document.currentScript
            requestAnimationFrame(() => scr.nextElementSibling.classList.add({', '.join(map(lambda s: f"'{s}'", node.classes))}));
        }}
    </script>
    '''
    translator.body.append(html_str)

class BreakNode(nodes.General, nodes.Element):
    pass

def visit_break_node_html(translator: SphinxTranslator, node: BreakNode) -> None:
    translator.body.append('<br />')

class ScriptInfo(SphinxDirective):
    required_arguments: int = 1
    final_argument_whitespace: bool = True
    has_content: bool = True
    option_spec: dict = {
        'type': unchanged_required,
        'pointer': unchanged,
        'initd': nonnegative_int_required,
        'initd_str': unchanged,
        'split_at': nonnegative_int,
    }
    def run(self) -> list[nodes.Node]:
        ty = self.options['type']
        pointer = self.options.get("pointer", None)
        initd = self.options['initd']
        initd_str = f'Available InitD[]: {initd}' if initd > 0 else 'No InitD[] available'
        if 'initd_str' in self.options:
            initd_str += f' {self.options["initd_str"]}'
        
        is_init_script: bool = ty == 'global' and 'Init' in self.arguments[0]
        
        scr_name = nodes.paragraph()
        if is_init_script:
            tystr = 'global script Init'
        else:
            tystr = f'{ty} script ScriptName'
        scr_name += ZScriptNode(data=tystr, inline=True)
        scr_name['classes'].append('scrinfo_dataline')
        if is_init_script:
            scr_name += [BreakNode(), ZScriptNode(data='@InitScript()', inline=True)]
        if pointer:
            ptr_name = nodes.paragraph(text=f'this-> pointer type: ')
            tynode = ZScriptNode(data=pointer, inline=True)
            tynode['classes'].append('hljs-type')
            ptr_name += tynode
        else:
            ptr_name = nodes.paragraph(text='no this-> pointer')
        ptr_name['classes'].append('scrinfo_dataline')
        initd_name = nodes.paragraph(text=initd_str)
        initd_name['classes'].append('scrinfo_dataline')
        
        sep_1 = nodes.paragraph()
        sep_2 = nodes.paragraph()
        sep_1['classes'].append('scrinfo_data_separator')
        sep_2['classes'].append('scrinfo_data_separator')
        
        data_column = nodes.container()
        
        data = [scr_name, sep_1, ptr_name, sep_2, initd_name]
        data_column += data
        data_column['classes'].append('scrinfo_databox')
        
        body_column = nodes.container()
        body_column['classes'].append('scrinfo_bodybox')
        
        main_row = nodes.container();
        main_row += [data_column, body_column]
        main_row['classes'].append('scrinfo_row')
        
        main_cont = nodes.container()
        title = nodes.paragraph(text=self.arguments[0])
        title['classes'].append('scrinfo_title')
        main_cont += [title, main_row]
        main_cont['classes'].append('scrinfo_card')
        
        body_nodes = self.parse_text_to_nodes('\n'.join(self.content), offset=self.content_offset)
        
        split_at = self.options.get('split_at', 0)
        if split_at > 0:
            if split_at >= len(body_nodes):
                self.reporter.error(f':split_at: is too high for the number of nodes in the content! ({split_at} >= {len(body_nodes)})')
            body_column += body_nodes[:split_at]
            body_below = nodes.container()
            body_below += body_nodes[split_at:]
            body_below['classes'].extend(['scrinfo_bodybox', 'kill_top_border'])
            body_column['classes'].append('kill_bottom_border')
            main_cont += [body_below]
        else:
            body_column += body_nodes
        
        return [main_cont]

def depart_ignored(translator: SphinxTranslator, node: nodes.Node) -> None:
    pass

def setup(app: Sphinx) -> ExtensionMetadata:
    app.add_directive('zscript', ZScriptDirective)
    app.add_directive('todo', TodoDirective)
    app.add_directive('plans', PlansDirective)
    app.add_directive('deprecated', ZSDeprecated, True)
    app.add_directive('style', Stylize)
    app.add_directive('scrinfo', ScriptInfo)
    
    app.add_node(
        ZScriptNode,
        html=(visit_zscript_node_html, depart_ignored),
        epub=(visit_zscript_node_unsupported, None),
        latex=(visit_zscript_node_unsupported, None),
        man=(visit_zscript_node_unsupported, None),
        texinfo=(visit_zscript_node_unsupported, None),
        text=(visit_zscript_node_unsupported, None),
        rinoh=(visit_zscript_node_unsupported, None),
    )
    app.add_node(
        TodoNode,
        html=(visit_admonition_node_html, depart_admonition_node_html),
        epub=(None, None),
        latex=(None, None),
        man=(None, None),
        texinfo=(None, None),
        text=(None, None),
        rinoh=(None, None),
    )
    app.add_node(
        PlansNode,
        html=(visit_admonition_node_html, depart_admonition_node_html),
        epub=(None, None),
        latex=(None, None),
        man=(None, None),
        texinfo=(None, None),
        text=(None, None),
        rinoh=(None, None),
    )
    app.add_node(
        StylizeNode,
        html=(visit_stylize_node_html, depart_ignored),
        epub=(None, None),
        latex=(None, None),
        man=(None, None),
        texinfo=(None, None),
        text=(None, None),
        rinoh=(None, None),
    )
    app.add_node(
        BreakNode,
        html=(visit_break_node_html, depart_ignored),
        epub=(None, None),
        latex=(None, None),
        man=(None, None),
        texinfo=(None, None),
        text=(None, None),
        rinoh=(None, None),
    )

    return {
        'version': '0.1',
        'env_version': 1,
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
