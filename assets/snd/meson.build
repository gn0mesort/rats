fs = import('fs')

assets = files('bump.mp3', 'attack.mp3', 'ouch.mp3', 'die.mp3', 'pickup.mp3', 'wakeup.mp3', 'theme.mp3')

foreach file : assets
  fs.copyfile(file, fs.name(file), install: true, install_dir: share / dirname / 'assets' / 'snd')
endforeach
