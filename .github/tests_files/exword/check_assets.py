import sys
from pathlib import Path

sys.path.insert(
    0,
    str(Path(sys.argv[1]).resolve()/'fxconv')
)

import fxconv

p = Path(__file__).resolve().parent
meta = fxconv.Metadata(path=str(p/'fxconv-metadata.txt'))

for f in p.glob('*.png'):
    params = meta.rules_for(str(f))
    data = fxconv.convert_topti(str(f),params) if params['type']=='font' else fxconv.convert_bopti_cg(str(f),params)
    assert data is not None
    print(f.name, 'conversion OK')
