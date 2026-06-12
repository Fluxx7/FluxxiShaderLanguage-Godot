// Smoke test for the FSL TextMate grammar.
//
// Usage:
//   cd editors
//   npm install --no-save vscode-textmate vscode-oniguruma
//   node test-grammar.js                      # tokenize every project/fsl/**.fsl file
//   node test-grammar.js path/to/file.fsl     # also print every token + its scopes
//
// A regex mistake in the grammar (bad escape, catastrophic pattern, etc.) shows up
// here as an ERROR instead of silently breaking highlighting in the editor.

const fs = require('fs');
const path = require('path');
const vsctm = require('vscode-textmate');
const oniguruma = require('vscode-oniguruma');

const GRAMMAR = path.join(__dirname, 'fsl-vscode/syntaxes/fsl.tmLanguage.json');
const FSL_DIR = path.join(__dirname, '../project/fsl');

const wasmBin = fs.readFileSync(
  path.join(__dirname, 'node_modules/vscode-oniguruma/release/onig.wasm')
).buffer;
const onigLib = oniguruma.loadWASM(wasmBin).then(() => ({
  createOnigScanner: (p) => new oniguruma.OnigScanner(p),
  createOnigString: (s) => new oniguruma.OnigString(s),
}));

const registry = new vsctm.Registry({
  onigLib,
  loadGrammar: async (scopeName) =>
    scopeName === 'source.fsl'
      ? vsctm.parseRawGrammar(fs.readFileSync(GRAMMAR, 'utf8'), GRAMMAR)
      : null,
});

function collectFslFiles(dir) {
  let out = [];
  for (const e of fs.readdirSync(dir, { withFileTypes: true })) {
    const p = path.join(dir, e.name);
    if (e.isDirectory()) out = out.concat(collectFslFiles(p));
    else if (e.name.endsWith('.fsl')) out.push(p);
  }
  return out;
}

function tokenizeFile(grammar, file, verbose) {
  const lines = fs.readFileSync(file, 'utf8').split('\n');
  let stack = vsctm.INITIAL;
  for (const line of lines) {
    const r = grammar.tokenizeLine(line, stack);
    if (verbose) {
      console.log(`LINE: ${line}`);
      for (const t of r.tokens) {
        const text = line.substring(t.startIndex, t.endIndex);
        if (text.trim() === '') continue;
        console.log(`  ${JSON.stringify(text)} -> ${t.scopes.slice(1).join(', ')}`);
      }
    }
    stack = r.ruleStack;
  }
  return lines.length;
}

(async () => {
  const grammar = await registry.loadGrammar('source.fsl');
  if (!grammar) {
    console.error('FAILED to load grammar');
    process.exit(1);
  }

  const target = process.argv[2];
  if (target) {
    tokenizeFile(grammar, target, true);
    return;
  }

  for (const file of collectFslFiles(FSL_DIR)) {
    const n = tokenizeFile(grammar, file, false);
    console.log(`OK: tokenized ${path.relative(process.cwd(), file)} (${n} lines)`);
  }
})().catch((e) => {
  console.error('ERROR:', e);
  process.exit(1);
});
