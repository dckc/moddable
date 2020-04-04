/* global trace */

let recur = 0;

function infiniteRecursion() { trace(`call stack depth: ${++recur}\n`); infiniteRecursion(); }

try {
    infiniteRecursion();
} catch (oops) {
    trace(`caught ${oops.message}! BWAHAHAH\n`);
}
