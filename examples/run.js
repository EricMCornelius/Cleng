#!/usr/bin/env node

var child_process = require('child_process');
var path = require('path');

var cmd = 'clang-3.5 %file% -fsyntax-only -Xclang -load -Xclang ../lib/libClengPlugin.so -Xclang -plugin -Xclang cleng'
cmd = cmd.replace(/%file%/g, process.argv[2]);

cmd += ' ' + process.argv.slice(3).join(' ');

var sep = cmd.split(' ');

console.log(sep.join(' '));

child_process.spawn(sep.shift(), sep, { stdio: 'inherit' });
