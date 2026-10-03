{
  uses_user_defaults: true,
  project_name: 'resonance',
  project_type: 'c++',
  description: 'Reconstructed source of the PlayStation 2 game FreQuency.',
  keywords: ['decompilation', 'frequency', 'game', 'playstation 2', 'reverse engineering'],
  want_codeql: false,
  want_tests: false,
  want_winget: false,
  clang_format_args: "$(git ls-files 'src/*.h' 'src/*.cpp' ':!:src/python/*')",
  clang_format+: {
    BreakInheritanceList: 'AfterColon',
    IncludeBlocks: 'Regroup',
    IncludeCategories: [
      {
        CaseSensitive: true,
        Priority: 1,
        Regex: '^<[a-z]',
      },
      {
        CaseSensitive: true,
        Priority: 2,
        Regex: '^<[A-Z][A-Za-z0-9]*/',
      },
      {
        CaseSensitive: true,
        Priority: 3,
        Regex: '^<[A-Z][^/]*>',
      },
      {
        CaseSensitive: true,
        Priority: 4,
        Regex: '^"',
      },
    ],
    ReflowComments: true,
  },
  package_json+: {
    cspell+: {
      ignorePaths+: ['3rdparty/**', 'src/python/PC/**', 'src/python/patches/**'],
    },
    'markdownlint-cli2'+: {
      ignores: ['3rdparty/**'],
    },
  },
  pre_commit_config+: {
    exclude: '^src/python/(PC|patches)/',
  },
  gitattributes+: ['/3rdparty/** -text linguist-vendored'],
  gitignore+: ['*.iso', '/.sbclaude-venv/'],
  // Vendored upstream sources are not reformatted.
  prettierignore+: ['/3rdparty/'],
  vscode+: {
    c_cpp+: {
      configurations: [
        {
          cStandard: 'gnu23',
          compilerPath: '/usr/bin/gcc',
          cppStandard: 'gnu++23',
          defines: ['VERSION="unknown"'],
          includePath: [
            '${workspaceFolder}/compat/**',
            '${workspaceFolder}/sce/**/include/**',
            '${workspaceFolder}/src/**',
            '${workspaceFolder}/.wiswa-ci/**/include/**',
          ],
          name: 'Linux',
        },
      ],
    },
  },
}
