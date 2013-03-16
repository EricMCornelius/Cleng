#!/usr/bin/env node

var child_process = require('child_process');
var path = require('path');

var cmd = 'clang++ %file% -fsyntax-only -Xclang -load -Xclang ../lib/libClengPlugin.so -Xclang -plugin -Xclang cleng -Xclang -plugin-arg-cleng -Xclang json -Xclang -plugin-arg-cleng -Xclang %arg%'
cmd = cmd.replace(/%file%/g, process.argv[2])
         .replace(/%arg%/g, process.argv[3]);
         
cmd += ' ' + process.argv.slice(4).join(' ');

var sep = cmd.split(' ');

console.log(sep.join(' '));

child_process.spawn(sep.shift(), sep, { stdio: 'inherit' });
