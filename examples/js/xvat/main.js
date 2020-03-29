import VM from "xmachine";

export default function main() {
    const vatMachine = new VM();
    (1, eval)("1");
    const result = vatMachine.evaluate("[1,2,3]");
    trace(`result: ${result}\n`);
    try {
        const result2 = vatMachine.evaluate("(function infiniteRecursion() { infiniteRecursion(); })()");
        //const result2 = vatMachine.evaluate("throw new TypeError()");
        trace(`result 2: ${result2}\n`);    
    } catch (oops) {
        trace(`survived: ${oops.message}\n`)
    }
}
