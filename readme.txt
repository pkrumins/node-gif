This is a node.js module, writen in C++, that uses giflib to produce GIF images
from RGB, BGR, RGBA or BGRA buffers.

It was written by Peteris Krumins (peter@catonmat.net).
His blog is at http://www.catonmat.net  --  good coders code, great reuse.

------------------------------------------------------------------------------

This module exports `Gif` and `DynamicGifStack` objects.


Gif
---

The `Gif` object is for creating simple GIF images. Gif's constructor takes
takes 5 arguments:

    var gif = new Gif(buffer, width, height, quality, buffer_type);

The first argument, `buffer`, is a node.js `Buffer` that is filled with RGB,
BGR, RGBA or BGRA values.
The second argument is integer width of the image.
The third argument is integer height of the image.
The fourth argument is the quality of output image.
The fifth argument is buffer type, 'rgb', 'bgr', 'rgba' or 'bgra'.

You can set the transparent color for the image by using:

    gif.setTransparencyColor(red, green, blue);

Once you have constructed Gif object, call `encode` method to encode and
produce GIF image.

    var image = gif.encode();



See `tests/gif.js` for a concrete example.


DynamicGifStack
---------------

The `DynamicGifStack` is for creating space efficient stacked GIF images. This  
object doesn't take any dimension arguments because its width and height is
dynamically computed. To create it, do:

    var dynamic_gif = new DynamicGifStack(buffer_type);

The `buffer_type` again is 'rgb', 'bgr', 'rgba' or 'bgra', depending on what type
of buffers you're gonna push to `dynamic_gif`.

It provides several methods - `push`, `encode`, `dimensions`, `setTransparencyColor`.

The `push` method pushes the buffer to position `x`, `y` with `width`, `height`.

The `encode` method produces the final GIF image.

The `dimensions` method is more interesting. It must be called only after
`encode` as its values are calculated upon encoding the image. It returns an
object with `width`, `height`, `x` and `y` properties. The `width` and
`height` properties show the width and the height of the final image. The `x`
and `y` propreties show the position of the leftmost upper PNG.

Here is an example that illustrates it. Suppose you wish to join two GIFs
together. One with width 100x40 at position (5, 10) and the other with
width 20x20 at position (2, 210). First you create the DynamicGifStack object:

    var dynamic_gif = new DynamicGifStack('rgb');

Next you push the RGB buffers of the two GIFs to it:

    dynamic_gif.push(gif1_buf, 5, 10, 100, 40);
    dynamic_gif.push(gif2_buf, 2, 210, 20, 20);

Now you can call `encode` to produce the final GIF:

    var image = dynamic_gif.encode();

Now let's see what the dimensions are,

    var dims = dynamic_gif.dimensions();

The x position `dims.x` is 2 because the 2nd GIF is closer to the left.
The y position `dims.y` is 10 because the 1st GIF is closer to the top.
The width `dims.width` is 103 because the first GIF stretches from x=5 to
x=105, but the 2nd GIF starts only at x=2, so the first two pixels are not
necessary and the width is 105-2=103.
The height `dims.height` is 220 because the 2nd GIF is located at 210 and
its height is 20, so it stretches to position 230, but the first GIF starts
at 10, so the upper 10 pixels are not necessary and height becomes 230-10=220.

See `tests/dynamic-gif-stack.js` for a concrete example.


How to Install?
---------------

To compile the module, make sure you have giflib [1] and run:

    node-waf configure build

This will produce gif.node object file. Don't forget to point NODE_PATH to
node-gif directory to use it.

Another way to get it installed is to use node.js packaga manager npm [2]. To
get node-gif installed via npm, run:

    npm install gif

This will take care of everything and you don't need to worry about NODE_PATH.

[1]: http://sourceforge.net/projects/giflib/


Wondering about PNG or JPEG?
----------------------------

Wonder no more, I also wrote modules to produce PNG and JPEG images.
Here they are:

    http://github.com/pkrumins/node-png
    http://github.com/pkrumins/node-jpeg


Why was this module written?
----------------------------

This module was written for my and James Halliday startup StackVM. StackVM puts
virtual machines on the web!

More info about our startup at: http://www.stackvm.com
Our startup is open source: http://github.com/pkrumins/stackvm

James Halliday is at http://substack.net and http://github.com/substack

------------------------------------------------------------------------------

Have fun with this module!


Sincerely,
Peteris Krumins (@pkrumins at Twitter)
http://www.catonmat.net

