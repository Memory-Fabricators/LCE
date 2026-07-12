if host_machine.system() == 'windows'
  cdata.set('SDL_AUDIO_DRIVER_DSOUND', 1)
  #  cdata.set('SDL_AUDIO_DRIVER_WASAPI', 1)
  subdir('directsound')
  #  subdir('wasapi')
elif host_machine.system() == 'darwin'
  cdata.set('SDL_AUDIO_DRIVER_COREAUDIO', 1)
  subdir('coreaudio')
else
  # Trimmed from upstream/wrapdb: this project drives all sound through
  # OpenAL (openal-soft, via subprojects/openal-soft.wrap), never through
  # SDL_audio, so the real Linux backend (pipewire, which otherwise pulls in
  # libpipewire-0.3 as a hard system dependency) is unnecessary - the dummy
  # backend is enough to satisfy SDL_audio's always-compiled entry points.
  cdata.set('SDL_AUDIO_DRIVER_DUMMY', 1)
  subdir('dummy')
endif

sources += files(
  'SDL_audio.c',
  'SDL_audiocvt.c',
  'SDL_audiodev.c',
  'SDL_audioqueue.c',
  'SDL_audioresample.c',
  'SDL_audiotypecvt.c',
  'SDL_mixer.c',
  'SDL_wave.c',
)
