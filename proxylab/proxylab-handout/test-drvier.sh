#!/bin/sh
# 测试脚本
PROXY_PORT=12345
SERVER_PORT=12356

echo "start proxy..."
./proxy ${PROXY_PORT} &

echo "start server..."
./tiny/tiny ${SERVER_PORT} &

echo "start test"
curl -v http://127.0.0.1:${PROXY_PORT}/home.html