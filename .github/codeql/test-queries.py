"""Compare standard and compact format-string findings on unsafe and safe calls."""

import argparse
from collections import Counter
import json
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile


def read_json(command):
  result = subprocess.run(command, check=True, capture_output=True, text=True)
  return json.loads(result.stdout)


def matches(codeql, database, query):
  result = read_json([codeql, 'bqrs', 'decode', str(database / 'results' / query),
                      '--format=json', '--entities=all', '--result-set=#select'])

  def entity(value):
    return value['label'], json.dumps(value.get('url'), sort_keys=True)

  return Counter((entity(row[0]), row[3], entity(row[4]), row[5])
                  for row in result['tuples'])


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('--codeql', required=True)
  parser.add_argument('--queries', required=True, type=Path)
  parser.add_argument('--work-dir', required=True, type=Path)
  args = parser.parse_args()
  args.work_dir.mkdir(parents=True, exist_ok=True)
  directory = Path(tempfile.mkdtemp(prefix='format-string-', dir=args.work_dir))
  source = directory / 'format-string.c'
  shutil.copyfile(Path(__file__).parent / 'tests/format-string.c', source)
  database = directory / 'database'
  command = shlex.join(['gcc', '-c', str(source), '-o', str(directory / 'format-string.o')])
  subprocess.run([args.codeql, 'database', 'create', str(database), '--language=cpp',
                  f'--source-root={directory}', f'--command={command}',
                  '--threads=2', '--ram=6000'], check=True)
  relative = 'Security/CWE/CWE-134/UncontrolledFormatString.ql'
  resolved = read_json([args.codeql, 'resolve', 'queries', '--format=bylanguage',
                        'codeql/cpp-queries:codeql-suites/cpp-code-scanning.qls'])
  candidates = [path for path in resolved['byLanguage']['cpp'] if path.endswith('/' + relative)]
  if len(candidates) != 1:
    raise RuntimeError('Cannot identify the standard format-string query.')
  for name, query in [('standard', candidates[0]),
                       ('compact', str(args.queries / 'UncontrolledFormatString.ql'))]:
    subprocess.run([args.codeql, 'database', 'analyze', str(database), query,
                    '--format=sarif-latest', f'--output={directory / (name + ".sarif")}',
                    '--threads=2', '--ram=6000'], check=True)
  standard = matches(args.codeql, database,
                      Path('codeql/cpp-queries') / relative.replace('.ql', '.bqrs'))
  compact = matches(args.codeql, database,
                     Path('resonance/codeql-queries/UncontrolledFormatString.bqrs'))
  if standard != compact or not standard:
    raise RuntimeError('The compact query changed the detected source and sink matches.')
  for name in ['standard', 'compact']:
    results = json.loads((directory / (name + '.sarif')).read_text())['runs'][0]['results']
    if len(results) != 3:
      raise RuntimeError(f'{name} must report the three unsafe calls and omit the safe calls.')
    if name == 'compact' and any(not result.get('codeFlows') for result in results):
      raise RuntimeError('A compact finding has no explanation path.')
  print('All source and sink matches agree; three unsafe calls retain explanation paths.')


if __name__ == '__main__':
  main()
