{
  scope: 'Host tools under tools/. PS2 target sources require the EE compiler.',
  configure_command: 'cmake -S tools -B build-scan -DCMAKE_BUILD_TYPE=Debug',
  documentation_steps: [
    {
      name: 'Install dependencies',
      run: 'sudo apt-get update\nsudo apt-get install -y doxygen graphviz\n',
    },
    {
      name: 'Configure',
      run: 'cmake -B "$GITHUB_WORKSPACE/build" -DBUILD_DOCS_ONLY=ON',
    },
    {
      name: 'Build documentation',
      run: 'cmake --build "$GITHUB_WORKSPACE/build" --target docs',
    },
    {
      name: 'Page count',
      run: "COUNT=$(find \"$GITHUB_WORKSPACE/build/html\" -name '*.html' | wc -l)\necho \"Generated $COUNT page(s)\"\ntest \"$COUNT\" -gt 0\n",
    },
  ],
  env: {},
  setup_steps: [
    {
      name: 'Install build dependencies',
      run: 'sudo apt-get update\nsudo apt-get install -y libssl-dev zlib1g-dev\n',
    },
    {
      name: 'Install current CMake',
      run: 'pipx install cmake',
    },
  ],
  workflow_file: 'pages.yml',
}
