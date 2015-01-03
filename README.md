lmv - Lindenmayer System Viewer
===============================

Copyright (c) 2014 Will Roberts \<wildwilhelm@gmail.com\>

Homepage: http://amor.cms.hu-berlin.de/~robertsw/lm.html

Source: https://github.com/wroberts/lmv

This project is licensed under the terms of the MIT license (see
LICENSE.md).

## Description

I'm reposting a little OpenGL program I made years ago, along with some images it can generate. Sierpinksi gasket: 

[![Sierpinski Gasket (click to embiggen)](https://raw.githubusercontent.com/wroberts/lmv/master/img/sierpinski_sm.png)](https://raw.githubusercontent.com/wroberts/lmv/master/img/sierpinski.png)

Sierpinski carpet:

[![Sierpinski Carpet (click to embiggen)](https://raw.githubusercontent.com/wroberts/lmv/master/img/sponge_sm.png)](https://raw.githubusercontent.com/wroberts/lmv/master/img/sponge.png)

Large sample of L-Systems inspired by [Lindenmayer’s book](http://algorithmicbotany.org/papers/#abop):

[![L-System Mosaic (click to embiggen)](https://raw.githubusercontent.com/wroberts/lmv/master/img/large_sm.png)](https://raw.githubusercontent.com/wroberts/lmv/master/img/large.png)

I think I originally wrote this for fun, but I ended up using it for an undergraduate computer science presentation.

## Building

The project uses the [GNU Autotools](https://www.gnu.org/software/automake/) as a build system, so building should work with `./configure ; make`. 
