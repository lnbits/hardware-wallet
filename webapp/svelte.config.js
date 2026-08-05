import adapter from '@sveltejs/adapter-static'

const config = {
  kit: {
    adapter: adapter({ fallback: '404.html' }),
    csp: {
      mode: 'hash',
      directives: {
        'default-src': ['self'],
        'script-src': ['self'],
        'style-src': ['self'],
        'img-src': ['self', 'data:', 'blob:'],
        'font-src': ['self'],
        'connect-src': ['self', 'https:', 'http:', 'wss:', 'ws:'],
        'object-src': ['none'],
        'base-uri': ['none'],
        'form-action': ['none'],
        'frame-src': ['none'],
        'worker-src': ['none'],
      },
    },
    paths: {
      base: process.env.BASE_PATH || '',
    },
  },
}

export default config
