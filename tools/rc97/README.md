# RC97: optimised codegen (CPU)

1. `rexglue codegen` with `cr_as_local`, `ctr_as_local`, `xer_as_local`, `non_volatile_as_local` in `[entrypoint]`
   and `config/ds2_overrides.share.toml` in `includes` (after the other five).
2. `python3 lee_antes_ds2.py <gen>`: functions that read r14-r31/cr/ctr/xer before writing them. Ignore
   `setFromMask`, `const auto s_` and the `stvx128` prologue saves; mark the rest with `mark.py <list> config`
   (`share_registers = true`) and generate again until nothing is left.
3. `directas_ds2.py <gen>`: calls to functions without hooks become `__imp__sub_X` (inlinable).
4. `novolatile.py <gen>`: guest loads/stores without volatile (MMIO keeps it); compiler barrier on every loop
   back-edge and at sync/lwsync/eieio/isync.
Steps 3 and 4 must be repeated after every codegen.
