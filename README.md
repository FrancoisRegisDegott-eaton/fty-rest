# fty-rest

## How to build

To build, run:

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=usr -DBUILD_TESTING=On ..
make
sudo make install
```

## Protocols

### STREAM DELIVER

#### SSE generic message

The SSE servlet consumes generic messages on the STREAM-DELIVER stream.

The message subject *must* be "SSE".

The message shall be frame formated as:

TOPIC/JSON\_PAYLOAD[/ASSET\_INAME]

where:

* TOPIC is a topic identifier
* JSON\_PAYLOAD is a JSON well-formed object
* ASSET\_INAME is an asset internal name (optional frame)
