# Bowser HWW web installer

## Test locally

The installer uses shared styles and images from `lnbits.github.io`. From the
root of this repository, start a local server that serves repository files
first and proxies missing shared assets to the deployed LNbits site:

```bash
npm exec --yes http-server -- . \
  -p 8000 \
  -a 127.0.0.1 \
  -c-1 \
  -P https://lnbits.github.io
```

Open the installer in Chrome, Chromium, or another Web Serial-compatible
browser:

<http://localhost:8000/installer/>

Use `localhost` rather than `0.0.0.0` so the browser treats the page as a
secure context and permits Web Serial. Keep an internet connection while
testing so the server can retrieve the shared LNbits assets.

A plain `python3 -m http.server` serves the local installer files, but cannot
provide the shared `/assets` files used by the GitHub Pages site, so the page
will appear unstyled.

Press `Ctrl+C` in the terminal to stop the server.
