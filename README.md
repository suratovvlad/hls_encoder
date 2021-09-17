# hls encoder

Sample application for encoding media files to HLS VOD media for streaming.

It takes first mp4 file as a source for video stream, second mp4 file as a source for audio stream,
transcodes video stream with scaling muxing it with audio stream to HLS VOD media stream.

The result media is presented by master playlist, media playlists and segment files grouped by scaled resolution.

# Usage

```
./hls_encoder /path/to/test_video_track.mp4 /path/to/test_audio_track.mp4 /path/to/output/directory/
```

# Tuning parameters

Unfortunately current version does not support parametrizing video scales by program arguments,
however it is possible to do with small modification of main.cpp like it is described below:

```cpp
....
// Setup desired scaling parameters for output video stream
scaling_options _scale_1 = {
        1920, 1080,                             // source
        1280, 720,                              // target
        AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P, // pixel formats of source and target
        2 * 1000 * 1000 - AUDIO_BITRATE,        // bitrate, 128000 - audio bitrate
        60,                                     // GOP size
        1                                       // max_b_frames
};

// And add it to the _hls_encoder
_hls_encoder->add_scaling_option( _scale_1 );

// Initialization and start transcoding
_hls_encoder->initialize_input_output();
_hls_encoder->process_transcoding();
....
```

Configure all needed resolutions, compile application and execute it.

# Build

Only linux is supported right now.

Install cmake, ninja and ffmpeg-devel packages (it depends on distro). Then execute next commands:

```
mkdir build
cd build
cmake -G "Ninja" ../
ninja
```

# How to run local HLS server

Edit hls_server/hls-server.js with desired port number and location of the stream mediafiles
```javascript
var HLSServer = require('hls-server')
var http = require('http')

var server = http.createServer()
var hls = new HLSServer(server, {
    path: '/streams',     // Base URI to output HLS streams
    dir: '/path/to/output/directory/'  // Directory that input files are stored
})
server.listen( 8001 )
```

Install node js with npm package manager (it also depends on distro). Then install hls-server package
```
cd hls_server
npm install hls-server
```

Then start server application
```
node hls-server.js
```

# Check your stream with hls.js demo

Go to the address
```
https://hls-js.netlify.app/demo/
```
Put the address of the stream into the input box
```
http://localhost:8001/streams/master.m3u8
```

Make sure video is loaded and could be played.

If you experience CORS issues, run browser in unsafe mode:

```
google-chrome-stable --disable-web-security --user-data-dir=~/chromeTemp -–allow-file-access-from-files
```