# Common webserv test commands

Assumes default config (`default.conf`) binds `127.0.0.1:8080`.

## Start/stop server

```zsh
cd /home/mipinhei/Desktop/Common_Core/webserv_git
./webserv default.conf
```

If ports are stuck:
```zsh
ss -ltnp '( sport = :8080 or sport = :8081 )' || true
pgrep -f '(^|/)webserv(\s|$)' || true
```

## Quick GET/HEAD

```zsh
curl -v --http1.1 http://127.0.0.1:8080/
curl -v --http1.1 http://127.0.0.1:8080/index.html
curl -I  --http1.1 http://127.0.0.1:8080/
```

## Redirect (directory without trailing slash)

```zsh
curl -v --http1.1 http://127.0.0.1:8080/www
```

## 404 / forbidden examples

```zsh
curl -v --http1.1 http://127.0.0.1:8080/does-not-exist
```

## Raw upload (POST /upload/<filename>)

```zsh
printf 'hello\n' | curl -v --http1.1 --data-binary @- \
  http://127.0.0.1:8080/upload/raw_hello.txt

# verify
cat /home/mipinhei/Desktop/Common_Core/webserv_git/www/uploads/raw_hello.txt
```

## Multipart upload (POST /upload)

```zsh
printf 'multipart-content\n' > /tmp/up.txt
curl -v --http1.1 -F 'file=@/tmp/up.txt;filename=from_multipart.txt' \
  http://127.0.0.1:8080/upload

# verify
cat /home/mipinhei/Desktop/Common_Core/webserv_git/www/uploads/from_multipart.txt
```

## Chunked POST via nc (authoritative chunk framing)

```zsh
{ printf 'POST /upload/chunked_nc.txt HTTP/1.1\r\nHost: 127.0.0.1:8080\r\nTransfer-Encoding: chunked\r\nContent-Type: application/octet-stream\r\nConnection: close\r\n\r\n'
  printf '6\r\nhello\n\r\n'
  printf '6\r\nworld!\r\n'
  printf '0\r\n\r\n'
  sleep 1
} | nc 127.0.0.1 8080

# verify
xxd -g 1 /home/mipinhei/Desktop/Common_Core/webserv_git/www/uploads/chunked_nc.txt
```

## Chunked POST via curl (streaming upload)

This is the most reliable curl shape to get chunked (size unknown):

```zsh
printf 'hello\nworld!\n' | curl --http1.1 -v \
  -X POST -T - \
  -H 'Content-Type: application/octet-stream' \
  -H 'Expect:' \
  http://127.0.0.1:8080/upload/chunked_curl.txt
```

Confirm what curl actually sent:
```zsh
printf 'hello\nworld!\n' | curl --http1.1 \
  -X POST -T - \
  -H 'Content-Type: application/octet-stream' \
  -H 'Expect:' \
  --trace-ascii /tmp/curl_trace.txt \
  http://127.0.0.1:8080/upload/chunked_curl.txt

grep -iE 'Transfer-Encoding:|Content-Length:' /tmp/curl_trace.txt
```

## DELETE an uploaded file

```zsh
curl -v --http1.1 -X DELETE http://127.0.0.1:8080/upload/raw_hello.txt
```

## Run the existing edge test suite

`edge_tests.sh` sends many raw HTTP cases (including chunked) via `nc`.

```zsh
cd /home/mipinhei/Desktop/Common_Core/webserv_git
bash edge_tests.sh 127.0.0.1 8080
```
