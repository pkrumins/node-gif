var fs  = require('fs');
var sys = require('sys');
var Gif = require('gif').Gif;
var Buffer = require('buffer').Buffer;

var terminal = fs.readFileSync('./terminal.rgb');

var gif = new Gif(terminal, 720, 400, 'rgb').encodeSync();

fs.writeFileSync('./terminal.gif', gif.toString('binary'), 'binary');

