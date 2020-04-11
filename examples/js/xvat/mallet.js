/* global trace */

trace(`mallet self: ${self}\n`);

self.onmessage = (msg) => {
  let recur = 0;

  function infiniteRecursion() { trace(`call stack depth: ${++recur}\n`); infiniteRecursion(); }

  try {
    infiniteRecursion();
  } catch (oops) {
    trace(`mallet self: ${self}\n`);
    trace(`caught ${oops.message}! BWAHAHAH\n`);
  }
};

