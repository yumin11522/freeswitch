# mod_gmeet

作为GMeet会议客户端，支持推入合成画面流及将GMeet流加入合成画面中 [GMeet](https://gmeet.numax.tech).

## 邀请内部号码入会
    conference 8888-nuas_gcas bgdial user/1018

## 邀请 rtmp 入会 
    conference 8888-nuas_gcas bgdial vlc/rtmp://10.8.106.111/proxy/numax

## 会议室画面推流至 GMU (webrtc whip)
    conference 8888-nuas_gcas bgdial {video_use_audio_ice=true,rtp_payload_space=103,absolute_codec_string=PCMU\,H264,media_timeout=10000000,url=http://127.0.0.1:80/index/api/whip?app=live&stream=livestream}gmeet/auto_answer

## 会议室画面拉流 (webrtc whep)
    conference 8888-nuas_gcas bgdial {video_use_audio_ice=true,rtp_payload_space=103,absolute_codec_string=PCMU\,H264,media_timeout=10000000,url=http://127.0.0.1:80/index/api/whep?app=live&stream=test}gmeet/auto_answer

## 会议室播放 MP4 文件 ( async 可选参数 )
    conference 8888-nuas_gcas play /tmp/genew_opus.mp4 async

## 会议室拉 RTSP、RTMP、HTTP
    conference 8888-nuas_gcas play av://rtsp://10.8.106.111/proxy/numax
    conference 8888-nuas_gcas play av://rtmp://10.8.106.111/live/test
    conference 8888-nuas_gcas play av://http://10.8.106.111/live/test.flv

## 结束会议 
    conference 8888-nuas_gcas hup all
