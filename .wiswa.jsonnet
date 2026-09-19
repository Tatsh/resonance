{
  uses_user_defaults: true,
  project_name: 'resonance',
  project_type: 'c++',
  description: 'Reconstructed source of the PlayStation 2 game FreQuency.',
  keywords: ['decompilation', 'frequency', 'game', 'playstation 2', 'reverse engineering'],
  want_codeql: false,
  want_tests: false,
  clang_format_args: 'include/*/*.h include/*/*/*.h src/*.cpp src/*/*.cpp src/*/*/*.cpp',
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
      ignorePaths+: ['src/python/PC/**', 'src/python/patches/**'],
    },
  },
  pre_commit_config+: {
    exclude: '^src/python/(PC|patches)/',
  },
}
