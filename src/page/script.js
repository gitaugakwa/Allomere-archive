(function () {
  var audio = document.querySelector("#player");

  if (Hls.isSupported()) {
    var hls = new Hls();
    // hls.loadSource(
    //   "https://stream.ram.radio/audio/ram.stream_aac/playlist.m3u8"
    // );
    hls.loadSource("http://localhost:5000/radio/hls/live.m3u8");
    hls.attachMedia(audio);
  }

  plyr.setup(audio);
})();
