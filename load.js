#!/usr/bin/env node

var repl = require('repl');

var inst = repl.start({});
inst.context.doc = require(process.argv[2] || './output.json');
