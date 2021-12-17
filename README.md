# wf-globalgestures

Global touchpad gestures plugin for [Wayfire]: implements a special protocol
(also in this repo) that lets clients request that a particular
gesture with a particular number of fingers always be delivered to
their surface.

This allows for implementation of layer-shell based desktop UI elements that
smoothly slide/fade in in response to particular gestures.

[Wayfire]: https://github.com/WayfireWM/wayfire

## Usage

Just install and add to the list. There is no configuration.

## Development

See `wfp-global-gestures-unstable-v1.xml` and `sample.c`.
It's not fancy (there's not even animations finishing the transitions) but it's a sample
implementing a layer-shell surface that becomes non-transparent and input-able when "pulled in"
onto the desktop with a 4-finger swipe-in.

## License

This is free and unencumbered software released into the public domain.  
For more information, please refer to the `UNLICENSE` file or [unlicense.org](https://unlicense.org).
