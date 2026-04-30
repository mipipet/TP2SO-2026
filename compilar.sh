docker start ARQUI-TPE
docker exec -it ARQUI-TPE make clean -C /root/Toolchain
docker exec -it ARQUI-TPE make clean -C /root/
docker exec -it ARQUI-TPE make -C /root/Toolchain
docker exec -it ARQUI-TPE make -C /root/
docker stop ARQUI-TPE
