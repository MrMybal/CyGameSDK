"""Install source fixtures without replacing existing user content."""
from pathlib import Path
import shutil
root=Path(__file__).resolve().parents[1]
new_assets={'StaticTriangle.cy3d','AnimatedTriangle.cy3d','Example_Data.json','Example_TexturedMaterial.cyasset','Grid_LOD.cy3d','TallTarget.cy3d','ChainMotion.cy3d'}
for source in (root/'app/examples').iterdir():
    if not source.is_file() or source.suffix not in {'.cyasset','.cy3d','.json','.png','.jpg','.jpeg','.bmp'}:continue
    target=root/'data/assets'/('BlueprintExamples' if source.name.startswith('Example_Blueprint') else 'Examples3D' if source.name in new_assets else 'Examples')
    target.mkdir(parents=True,exist_ok=True)
    destination=target/source.name
    try:
        with destination.open('xb') as output,source.open('rb') as input_stream:shutil.copyfileobj(input_stream,output)
    except FileExistsError:continue
    print(destination)
