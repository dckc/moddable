// @flow

declare module 'snapshot' {
    declare export class SnapshotFFI {
        constructor(): SnapshotFFI;

        dump(root: mixed, exits: mixed[]): ArrayBuffer;
        encodeSlot(root: mixed, exits: mixed[]): ArrayBuffer;
    }
}
