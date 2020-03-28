import VM from "xmachine";

function main() {
    const vatMachine = new VM();
    vatMachine.call("main", "[1,2,3]");
}

main();
