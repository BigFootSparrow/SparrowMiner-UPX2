###### Modified by BigFoot Sparrow
# XMR-Stak-UPX2 - Cryptonight All-in-One Mining Software

XMR-Stak-UPX2 is a universal Stratum pool miner. This miner supports CPUs, AMD and NVIDIA GPUs and can be used to mine the crypto currencies Monero, Aeon, UPX2 and many more Cryptonight coins.

## HTML reports
<img src="https://gist.githubusercontent.com/fireice-uk/2da301131ac01695ff79539a27b81d68/raw/4c09cdeee86f94df2e9dd86b927e64aded6184f5/xmr-stak-cpu-hashrate.png" width="260"> <img src="https://gist.githubusercontent.com/fireice-uk/2da301131ac01695ff79539a27b81d68/raw/4c09cdeee86f94df2e9dd86b927e64aded6184f5/xmr-stak-cpu-results.png" width="260"> <img src="https://gist.githubusercontent.com/fireice-uk/2da301131ac01695ff79539a27b81d68/raw/4c09cdeee86f94df2e9dd86b927e64aded6184f5/xmr-stak-cpu-connection.png" width="260">

## Video setup guide on Windows

[<img src="https://gist.githubusercontent.com/fireice-uk/3621b179d56f57a8ead6303d8e415cf6/raw/f572faba67cc9418116f3c1dfd7783baf52182ce/vidguidetmb.jpg">](https://youtu.be/YNMa8NplWus)
###### Video by Crypto Sewer

## Overview
* [Features](#features)
* [Supported altcoins](#supported-altcoins)
* [Download](#download)
* [Usage](doc/usage.md)
* [HowTo Compile](doc/compile.md)
* [FAQ](doc/FAQ.md)
* [Developer Donation](#default-developer-donation)
* [Developer PGP Key's](doc/pgp_keys.md)

## Features

- support all common backends (CPU/x86, AMD-GPU and NVIDIA-GPU)
- support all common OS (Linux, Windows and macOS)
- supports algorithm cryptonight for Monero (XMR) and cryptonight-light (AEON)
- easy to use
  - guided start (no need to edit a config file for the first start)
  - auto-configuration for each backend
- open source software (GPLv3)
- TLS support
- [HTML statistics](doc/usage.md#html-and-json-api-report-configuraton)
- [JSON API for monitoring](doc/usage.md#html-and-json-api-report-configuraton)

## Supported altcoins

Besides [Monero](https://getmonero.org), following coins can be mined using this miner:

- [Uplexa](https://www.uplexa.com)
- [Aeon](http://www.aeon.cash)
- [BBSCoin](https://www.bbscoin.xyz)
- [BitTube](https://coin.bit.tube/)
- [Conceal](https://conceal.network)
- [Graft](https://www.graft.network)
- [Haven](https://havenprotocol.com)
- [Lethean](https://lethean.io)
- [Masari](https://getmasari.org)
- [Plenteum](https://www.plenteum.com/)
- [QRL](https://theqrl.org)
- [Stellite](https://stellite.cash/)
- [TurtleCoin](https://turtlecoin.lol)
- [Zelerius](https://zelerius.org/)
- [X-CASH](https://x-network.io/)


If your prefered coin is not listed, you can choose one of the following algorithms:
- 124Kib scratchpad memory
    - cryptonight_upx2
- 256Kib scratchpad memory
    - cryptonight_turtle
- 1MiB scratchpad memory
    - cryptonight_lite
    - cryptonight_lite_v7
    - cryptonight_lite_v7_xor (algorithm used by ipbc)
- 2MiB scratchpad memory
    - cryptonight
    - cryptonight_gpu (for Ryo's 14th of Feb fork)
    - cryptonight_masari (used in 2018)
    - cryptonight_v7
    - cryptonight_v7_stellite
    - cryptonight_v8
    - cryptonight_v8_double (used by X-CASH)
    - cryptonight_v8_half (used by masari and stellite)
    - cryptonight_v8_reversewaltz (used by graft)
    - cryptonight_v8_zelerius
- 4MiB scratchpad memory
    - cryptonight_haven
    - cryptonight_heavy

Please note, this list is not complete and is not an endorsement.

## Download

You can find the latest releases and precompiled binaries on GitHub under [Releases](https://github.com/BigFootSparrow/SparrowMiner-UPX2/xmr-stak/releases).

## Default Developer Donation

By default, the miner will donate 0% of the hashpower.

If you want to donate directly to support further development, here is my wallet

BigFoot Sparrow:
```
UPX1pgTYdrD6QRBW8N3XDyRYYkDwnD9b39nnc3CeTS276j8girySEpuRn5paMsMq7wFtZ9imsg9ML5gxwmCT1R416jNhN1dYZk
```

Or to iriginal developers:

fireice-uk:

4581HhZkQHgZrZjKeCfCJxZff9E3xCgHGF25zABZz7oR71TnbbgiS7sK9jveE6Dx6uMs2LwszDuvQJgRZQotdpHt1fTdDhk
```
psychocrypt:

45tcqnJMgd3VqeTznNotiNj4G9PQoK67TGRiHyj6EYSZ31NUbAfs9XdiU5squmZb717iHJLxZv3KfEw8jCYGL5wa19yrVCn
```
                                                                                                    
                                  .:+o-                      -o+:.                                  
                              ./shddddy`                    `yddddhs/.                              
                           `/yhdddddddd+    .:+oooooo+:.    +ddddddddhy/`                           
                         -ohdddddddddddd/ /yhddddddddddhy/ /ddddddddddddho-                         
                      `:sdddddddddddddddd/+dddddddddddddd+/dddddddddddddddds:                       
                      /dddddddddddddddddddo:hddddddddddh:odddddddddddddddddddy:                     
                   -o:`+dddddddddddddddddddy:ohddddddho:ydddddddddddddddddddddds-                   
                 .-/hdy:/hddddddddddddddddddho:sdddds:ohddddddddddddddddddddddddhs.                 
               `+hhyshddyohdddddddddddddddddddh+:oo:+hddddddddddddddddddddddddddddh+`               
              :yddddddddddddddddddddddddddddddddhoohddddddddddddddddddddddddddddddddy:              
            .oddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddo             
           :ydddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddds-`.           
         `odddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddds::oyho`         
        -ydddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddy+oydddddy-        
      `/hdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddhhdddddddddh/       
     `oddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddo`     
     +hddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddh+     
      -+hddddddddddddddddddddddddddddddddddhhhhhhhhhhhhhhddddddddddddddddddddddddddddddddddh+-      
        `:ohhdddddddddddddddddddddddddhs+oys/-.```````..--:/oshhdddddddddddddddddddddddhho:`        
           `-/shhddddddddddddddhhyssssyyo+--://-`             `-+hhhhddddddddddddddhhs/-`           
               `.:+oyhhhdddddyo++++++/:--/oso/-.-`          `-+yy+-`-/ydddddhhhyo+:.`               
                     `.-::+dyyhy+/:::/+oyso/:://:.`     ``:oyy+-       /d+::-.`                     
                          -dd+`       .--.-:+oo++/-.``:+shs/.``        .d.                          
                        .ydds       :yddddhs/`  `-/oyhs/-`./oy-        `ddy.                        
                        :ddd+      -ddddddddddyhys+:.`-/shddddys/      .ddd-                        
                         yddy`     :ddddddddddd.    :hhhddddddy/+o-    :dds                         
                         :ddds`   `-ddddddddddo     -d+`ossss+`       .ydd-                         
                          ydddhosyys+/shdddho-      `+s.          `./oddds                          
                          :ddddhyys+/:-.````           `    `-:/+osshdddd-                          
                           hddddhs+:.``                      ``-:+ydddddy                           
                           +ddddyo/:::.`     .+++++++/.     `.:::/oydddd/                           
                           `.-odhs++:.      `-+shddhs+-`     `.:++shd+-.                            
                              .ddy+:.    -/shhddddddddhhs/-   `.:+ydh`                              
                               sdh+.`   +dddddhssoosshddddd+   `.ohds                               
                               odddh/` +ddddy/.`    `./ydddd/ `+hddd+                               
                               /dddddyoddddd+--/++++/--+dddddohddddd:                               
                               -dddddddddddddhddddddddhddddddddddddd.                               
                               `ddhhddddddddddddddddddddddddddddhhdd`                               
                                y+.odddddddddddddddddddddddddddd+.+s                                
                                `  /dddddddddddddddddddddddddddd:  `                                
                                   -dddhddddddddddddddddddddhddd.                                   
                                   `dh+.hddddddddddddddddddh.+hd`                                   
                                    s-  sddddddddddddddddddo  -o                                    
                                        /dddddddddddddddddd:                                        
                                        .dddyhddddddddhyddd`                                        
                                         hh/`/dddddddd:`+hy                                         
                                         /.   +dddddd/   .:                                         
                                               odddd+                                               
                                               `sdds                                                
                                                `yy`                                                
                                                 .`                                                 
                                                                                                    
