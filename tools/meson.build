libdir_abs = join_paths(get_option('prefix'), get_option('libdir'))
datadir_abs = join_paths(get_option('prefix'), get_option('datadir'))

tools = [
  ['g-ir-scanner', 'scannermain', 'scanner_main'],
  ['g-ir-annotation-tool', 'annotationmain', 'annotation_main'],
]
if get_option('doctool')
  tools += [['g-ir-doc-tool', 'docmain', 'doc_main']]
endif

python_name = 'python@0@'.format(python.language_version().split('.')[0])
tool_output = []
foreach tool : tools
  tools_conf = configuration_data()
  tools_conf.set('libdir', libdir_abs)
  tools_conf.set('datarootdir', datadir_abs)
  tools_conf.set('PYTHON', python_name)

  tools_conf.set('TOOL_MODULE', tool[1])
  tools_conf.set('TOOL_FUNCTION', tool[2])
  tool_bin = configure_file(
    input: 'g-ir-tool-template.in',
    output: tool[0],
    configuration: tools_conf,
    install: true,
    install_dir: get_option('bindir'),
  )
  tool_output += tool_bin
  # Provide tools for others when we're a subproject and they use the Meson GNOME module
  meson.override_find_program(tool[0], tool_bin)
endforeach

girscanner = tool_output[0]

gircompiler = executable('g-ir-compiler', 'compiler.c',
  dependencies: [
    girepo_internals_dep,
    girepo_dep,
  ],
  install: true,
)

girgenerate = executable('g-ir-generate', 'generate.c',
  dependencies: [
    girepo_internals_dep,
    girepo_dep,
  ],
  install: true,
)

girinspect = executable('g-ir-inspect', 'g-ir-inspect.c',
  dependencies: girepo_dep,
  install: true,
)
