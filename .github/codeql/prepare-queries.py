"""Prepare the current C++ security suite with compact format-string paths."""

import argparse
import json
from pathlib import Path
import re
import subprocess


def run_json(codeql, *arguments):
  result = subprocess.run([codeql, *arguments], check=True, capture_output=True, text=True)
  return json.loads(result.stdout)


def pack_version(root):
  match = re.search(r'^version:\s*[\'\"]?([^\s\'\"]+)',
                    (root / 'qlpack.yml').read_text(), re.MULTILINE)
  if not match:
    raise RuntimeError(f'Cannot identify the CodeQL pack version in {root}.')
  return match[1]


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('--codeql', required=True)
  parser.add_argument('--output', required=True, type=Path)
  parser.add_argument('--adapter', required=True, type=Path)
  args = parser.parse_args()
  suite = 'codeql/cpp-queries:codeql-suites/cpp-code-scanning.qls'
  resolved = run_json(args.codeql, 'resolve', 'queries', '--format=bylanguage', suite)
  baseline = set(resolved['byLanguage']['cpp'])
  relative = 'Security/CWE/CWE-134/UncontrolledFormatString.ql'
  candidates = [Path(path) for path in baseline if path.endswith('/' + relative)]
  if len(candidates) != 1:
    raise RuntimeError('The standard suite must include exactly one format-string query.')
  upstream = candidates[0]
  package = next(parent for parent in upstream.parents if (parent / 'qlpack.yml').is_file())
  packs = run_json(args.codeql, 'resolve', 'qlpacks', '--format=json')
  cpp_all = Path(packs['codeql/cpp-all'][0])
  original = upstream.read_text()
  if original.count('import Flow::PathGraph') != 1 or original.count('\nfrom\n') != 1:
    raise RuntimeError('The upstream query structure changed; review the graph adapter.')
  declarations, condition = original.rsplit('\nfrom\n', 1)[1].split('\nwhere\n', 1)
  condition = condition.split('\nselect ', 1)[0]
  declarations, count = re.subn(
    r',\s*Flow::PathNode sourceNode,\s*Flow::PathNode sinkNode\s*$', '', declarations)
  if count != 1:
    raise RuntimeError('The upstream query endpoints changed; review the graph adapter.')
  adapter = args.adapter.read_text()
  adapter = adapter.replace('REPORT_DECLARATIONS', declarations)
  adapter = adapter.replace('REPORT_CONDITION', condition)
  modified = original.replace('import Flow::PathGraph', 'import CompactPathGraph')
  modified = modified.replace('\nfrom\n', '\n' + adapter + '\nfrom\n')
  args.output.mkdir(parents=True, exist_ok=True)
  query = args.output / 'UncontrolledFormatString.ql'
  query.write_text(modified)
  (args.output / 'qlpack.yml').write_text(json.dumps({
    'name': 'resonance/codeql-queries',
    'version': '0.0.0',
    'dependencies': {
      'codeql/cpp-all': pack_version(cpp_all),
      'codeql/cpp-queries': pack_version(package),
    },
  }, indent=2, sort_keys=True) + '\n')
  output_suite = args.output / 'queries.qls'
  output_suite.write_text(json.dumps([
    {'import': 'codeql-suites/cpp-code-scanning.qls', 'from': 'codeql/cpp-queries'},
    {'exclude': {'query path': relative}},
    {'query': query.name},
  ], indent=2, sort_keys=True) + '\n')
  actual = run_json(args.codeql, 'resolve', 'queries', '--format=bylanguage', str(output_suite))
  actual = set(actual['byLanguage']['cpp'])
  expected = (baseline - {str(upstream)}) | {str(query)}
  if actual != expected:
    raise RuntimeError('The generated suite does not preserve the standard query selection.')
  print(f'Preserved all {len(baseline)} standard queries; compacted one explanation graph.')


if __name__ == '__main__':
  main()
