# Runtime audit

CorpseHighlighterF4 supports Fallout 4 1.10.163, 1.11.221 and 1.11.240 with one DLL.
Every explicit relocation is checked against the relevant Address Library
database and executable before deployment. The DLL repeats the executable-byte
checks before registering its death event sink.

| Binding | 1.10.163 ID / RVA | 1.11.221 ID / RVA | 1.11.240 ID / RVA |
|---|---:|---:|---:|
| death event source | 1465690 / `0x442550` | 2201833 / `0x531170` | 2201833 / `0x531490` |
| apply effect shader | 652173 / `0x422180` | 2205201 / `0x5E1E80` | 2205201 / `0x5E21A0` |
| player singleton | 303410 / `0x5AA4388` | 4798212 / `0x31E2DD0` | 4798212 / `0x31EDE50` |
| data-handler singleton | 711558 / `0x58CF080` | 4796135 / `0x30DC080` | 4796135 / `0x30E7100` |
| Scaleform global heap | 939898 / `0x6577EB0` | 2707353 / `0x3DA7500` | 2707353 / `0x3DBDD80` |
| GFx object add ref | 244786 / `0x20B9C30` | 2286228 / `0x1AD66B0` | 2286228 / `0x1AD6B70` |
| GFx object release | 856221 / `0x20B9C80` | 2286229 / `0x1AD66F0` | 2286229 / `0x1AD6BB0` |
| GFx set member | 1360149 / `0x20D05E0` | 2286589 / `0x1AE6A40` | 2286589 / `0x1AE6F00` |

The 1.11.240 executable has SHA-256
`fdcef37ac1230af6d0b0050eb2142b139ef3a867b37b9211fb6edfcc646072f8`.
Its entry signatures and F4SE 0.7.9 raw addresses were checked before enabling the
runtime in source. The published `version-1-11-240-0.bin` maps every listed
stable ID to the audited RVA.

Player and data-handler lookups use CommonLibF4's dual-runtime bindings; they are
included above because they are still Address Library dependencies.
