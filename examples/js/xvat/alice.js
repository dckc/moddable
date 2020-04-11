self.onmessage = (msg) => {
  for (let i = 0; i < 100000000; i++) {
  }
    self.postMessage("pong");
};
