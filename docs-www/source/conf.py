# TODO: apply for algolia license https://github.com/godotengine/godot-docs/issues/4461

import os
import sys

from pathlib import Path

import requests

script_dir = Path(os.path.dirname(os.path.realpath(__file__)))
root_dir = script_dir.parent.parent

sys.path.append(os.path.abspath('extensions'))

project = "ZQuest Classic"
copyright = "2024, ZQuest Classic Foundation"
author = "ZQuest Classic Developers"

extensions = ['zscript', 'sphinx_design']

templates_path = ["_templates"]
exclude_patterns = []

html_theme = "pydata_sphinx_theme"
html_static_path = ["_static"]
html_logo = "_static/ZC_Logo.png"
html_favicon = "_static/favicon.png"
html_css_files = ["main.css", "zscript.css"]
html_js_files = ["main.js", "highlight-11.11.1.min.js", "zscript.js"]

if not (root_dir / 'docs-www/source/_static/highlight-11.11.1.min.js').exists():
    content = requests.get('https://unpkg.com/@highlightjs/cdn-assets@11.11.1/highlight.min.js').text
    (root_dir / 'docs-www/source/_static/highlight-11.11.1.min.js').write_text(content)


html_theme_options = {
    # Collapse navigation (False makes it tree-like)
    "collapse_navigation": False,
}
html_show_sourcelink = False

rst_prolog = """
.. |const| replace:: :abbr:`const (This value cannot be reassigned. If it is an array, it's elements also cannot be reassigned.)`
.. |varargs| replace:: :abbr:`varargs (This method accepts any number of parameters after the ones described here.)`
.. |void| replace:: :abbr:`void (No return value.)`
.. |untyped| replace:: :abbr:`untyped (Could be any type.)`
.. |int| replace:: :abbr:`int (Not really an integer. In ZScript, int stores a fixed-position decimal number. For real integers, use the 'long' type.)`
.. |long| replace:: :abbr:`long (ZScript's real integer type.)`
.. |T| replace:: :abbr:`T (Generic type, can bound to any type.)`
.. |T1| replace:: :abbr:`T1 (Generic type, can bound to any type, as long as all generic types match.)`
.. |T2| replace:: :abbr:`T2 (Generic type, can bound to any type, as long as all generic types match.)`
.. |T3| replace:: :abbr:`T3 (Generic type, can bound to any type, as long as all generic types match.)`

.. |wip| replace:: WIP: This page is incomplete, and needs more information.
.. |stdlib| replace:: :ref:`standard library <lib_std>`
.. |VSCode| replace:: :ref:`VSCode<vscode>`
.. |ctc| replace:: :ref:`compile-time constant<compiletime_const>`

.. |qrs| replace:: :ref:`Quest Rules<globals_enum_qr>`
.. |qr| replace:: :ref:`Quest Rule<globals_enum_qr>`

.. role:: del

.. role:: zs(literal)
    :class: 'inlinezs pre'

.. role:: zs_num(literal)
    :class: 'customzs pre hljs hljs-number'

.. role:: zs_kw(literal)
    :class: 'customzs pre hljs hljs-keyword'

.. role:: zs_type(literal)
    :class: 'customzs pre hljs hljs-type'

.. role:: zs_str(literal)
    :class: 'customzs pre hljs hljs-string'

.. role:: zs_op(literal)
    :class: 'customzs pre hljs hljs-operator'

.. role:: zs_meta(literal)
    :class: 'customzs pre hljs hljs-meta'

.. role:: zs_cmnt(literal)
    :class: 'customzs pre hljs hljs-comment'

.. role:: zs_title(literal)
    :class: 'customzs pre hljs hljs-title'

.. default-role:: zs
"""

# pdf
rinoh_documents = [
    dict(doc='index', target='zquestclassic', template='pdf.rtt'),
]
