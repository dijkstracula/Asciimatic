Asciimatic - an edge-based ASCII art generator
============

This repository contains a cleaned-up mirror of the SVN repo for Nathan 
Taylor's final project in CPSC 505: Image Understanding, Fall 2009, at the 
University of British Columbia.

>Typically, tools to generate ASCII art from raster images, such as aalib or libcaca, quantize the input image to a series of brightness levels, and a printable ASCII character is then assigned to a corresponding subregion of the image, where a "darker" brightness level is assigned a "denser" character, and lighter brightness levels are assigned "sparser" characters. While simple to implement and fast enough for real-time purposes (both mplayer and VLC can display video as ASCII, and Quake has been modified to run in the terminal), it is difficult to see what the image is unless the characters are very small; additionally, ANSI colour information is often needed in order to improve the "shading" of the image.
>
>One can observe that more information than simply "brightness" can be encoded in an ASCII character; in particular, certain characters such as slashes, dashes, pipes, and other punctuation marks suggest strong direction. Also, the choice of character itself could suggest fine detail within the image (ie. a '+' when two near-parallel lines intersect, a 'O' for small circular features, etc.) Additionally, large swatches of similar colours look very busy to the eye; therefore, we may only want to register characters for regions of the image that correspond to changes in intensity; in other words, it is fruitful to consider an edge-based approach to ASCII-art generation.
>
>My CPSC 505 miniproject performs Canny edge-detection on an input in order to extract the primal sketch of an image. Additionally, each subregion of the Canny primal sketch is treated as a filter, and template matching is applied to a subset of printable ASCII characters in order to ask the question, "what character best approximates this region of interest?" The character that corresponds to the highest normalized cross corellation with the filter is outputted.
>
>Initially, the entire range of printable ASCII characters were used. However, this resulted in a very noisy output. Therefore, a subset was chosen by hand that perhaps best represents the kind of edges we would like to capture in text. Different characters could be chosen to represent different "shading types", at the artistic whim of the user.

Building and installation
-------------------------

$ make

Dependencies
------------

* cairo
* libconfig
* opencv
* qt

Note that qt is an optional dependency for opencv and that you might have to specify its inclusion yourself.
