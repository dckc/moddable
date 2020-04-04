self.onmessage = (ping) => {
    self.postMessage("bob pong:" + ping.toString());
}
