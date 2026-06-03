docker start TPE2-SO
docker exec -it TPE2-SO make clean -C /root/Toolchain
docker exec -it TPE2-SO make clean -C /root/
docker exec -it TPE2-SO make -C /root/Toolchain
docker exec -it TPE2-SO make -C /root/
docker stop TPE2-SO
