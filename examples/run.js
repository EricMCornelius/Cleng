#!/usr/bin/env node

var child_process = require('child_process');

// -internal-isystem /usr/local/include -internal-isystem /usr/bin/../lib/clang/3.1/include -internal-externc-isystem /include -internal-externc-isystem /usr/include

var cmd = 'clang';
var sources = [process.argv[2]];
var includes = ['/usr/local/include', '/usr/lib/clang/3.1/include', '/include', '/usr/include'];
var defines = ['_GNU_SOURCE', '_DEBUG', '__STDC_CONSTANT_MACROS', '__STC_FORMAT_MACROS', '__STDC_LIMIT_MACROS'];

includes = includes.map(function(path) { return '-I' + path; });
defines = defines.map(function(def) { return '-D' + def; });

var init = '-cc1 -x c'.split(' ');
var pluginArgs = ['-load', process.env['PWD'] + '/../plugin.so', '-plugin', 'cleng'];
var rest = process.argv.slice(3)
             .reduce(function(arr, arg) { arr = arr.concat(['-plugin-arg-cleng', arg]); return arr; }, []);

args = init.concat(pluginArgs).concat(rest).concat(sources).concat(includes);

child_process.spawn(cmd, args, { stdio: 'inherit' });
