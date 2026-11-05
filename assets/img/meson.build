fs = import('fs')

assets = files('cavewall.webp', 'stonefloor.webp', 'coins.webp', 'ruby.webp', 'ascii.webp', 'debug.webp',
               'reticle.webp', 'heart.webp', 'pie.webp', 'cheese.webp', 'bishop.webp')

foreach file : assets
  fs.copyfile(file, fs.name(file), install: true, install_dir: share / dirname / 'assets' / 'img')
endforeach
