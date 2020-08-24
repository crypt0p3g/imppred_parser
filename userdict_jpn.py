import struct
import datetime
import sys


# https://stackoverflow.com/questions/4869769/convert-64-bit-windows-date-time-in-python
def getFiletime(dt):
    microseconds = dt / 10
    seconds, microseconds = divmod(microseconds, 1000000)
    days, seconds = divmod(seconds, 86400)
    res = datetime.datetime(1601, 1, 1) + datetime.timedelta(days, seconds, microseconds)
    if res < datetime.datetime(1994, 1, 1) or res > datetime.datetime(2036, 1, 1):
        raise Exception
    return res.strftime("%Y.%m.%d (%H:%M:%S)")


def advanced_parse(filename):
    data_read = open(filename, 'rb').read()
    data = data_read[:]

    time, dwAllocatedSize, dwUnkFlag, nEntries, dwSizeUnk_0x20, dwUnk, dwUsedSize = struct.unpack('<QLLLLLL', data[0:0x20])
    print('Last Modified:', getFiletime(time))
    data = data[0x20:]

    for k in range(0, nEntries):
        time, dwSubSize, const_0x10, const_0x01, nSubEntryCount, const_0x00 = struct.unpack('<QHHccH', data[:16])
        data = data[16:]
        assert (const_0x10 == 0x10)
        assert (ord(const_0x01) == 0x01)
        assert (const_0x00 == 0x0)

        print(k, getFiletime(time), ":")
        iRealSubSize = 0x10
        for i in range(ord(nSubEntryCount)):
            dwSubStructSize, len_a, len_b, dwFlagsUnk2 = struct.unpack('<HccL', data[:8])
            data = data[8:]
            A = data[:ord(len_a) * 2].decode('utf-16le')
            data = data[ord(len_a) * 2:]
            B = data[:ord(len_b) * 2].decode('utf-16le')
            data = data[ord(len_b) * 2:]
            print('\t', A, '->', B)
            assert (dwSubStructSize == ord(len_b) * 2 + ord(len_a) * 2 + 8)
            iRealSubSize += dwSubStructSize
        assert (dwSubSize == iRealSubSize)

    while len(data):
        unparsed = b''
        for j in range(0, 0x20):
            data_saved = data[:]
            try:
                time, a1, a2, c1x, c1y, c2 = struct.unpack('<QHHccH', data[0:8 + 8])
                data = data[8 + 8:]
                print('SLACK', getFiletime(time), ":")
                for i in range(ord(c1y)):
                    d2, d1x, d1y, g = struct.unpack('<HccL', data[0:8])
                    data = data[8:]
                    A = data[:ord(d1x) * 2].decode('utf-16le')
                    data = data[ord(d1x) * 2:]
                    B = data[:ord(d1y) * 2].decode('utf-16le')
                    data = data[ord(d1y) * 2:]
                    if len(A) != 0 or len(B) != 0:
                        print('\t', A, '->', B)
                break
            except:
                if len(data):
                    unparsed += bytes([data_saved[0]])
                    data = data_saved[1:]
        if len(unparsed):
            print('UNPARSED', unparsed.decode('utf-16le', errors="ignore"))
            unparsed = b''


def simple_parse(filename):
    mapping = []

    data_read = open(filename, 'rb').read()
    data = data_read[:]

    time, dwAllocatedSize, dwUnkFlag, nEntries, dwSizeUnk_0x20, dwUnk, dwUsedSize = struct.unpack('<QLLLLLL', data[0:0x20])
    print('Last Modified:', getFiletime(time))
    data = data[0x20:]

    for k in range(0, nEntries):
        time, dwSubSize, const_0x10, const_0x01, nSubEntryCount, const_0x00 = struct.unpack('<QHHccH', data[:16])
        data = data[16:]

        assert (const_0x10 == 0x10)
        assert (ord(const_0x01) == 0x01)
        assert (const_0x00 == 0x0)

        iRealSubSize = 0x10
        text_out = ''
        text_in = ''
        for i in range(ord(nSubEntryCount)):
            dwSubStructSize, len_a, len_b, dwFlags = struct.unpack('<HccL', data[:8])
            data = data[8:]
            A = data[:ord(len_a) * 2].decode('utf-16le')
            data = data[ord(len_a) * 2:]
            B = data[:ord(len_b) * 2].decode('utf-16le')
            data = data[ord(len_b) * 2:]
            text_out += B
            if B == '':
                text_out += A
            text_in += A
            assert (dwSubStructSize == ord(len_b) * 2 + ord(len_a) * 2 + 8)
            iRealSubSize += dwSubStructSize

        mapping += [(getFiletime(time), text_out)]
        assert (dwSubSize == iRealSubSize)
    mapping = sorted(mapping, key=lambda x: x[0])
    for x in mapping:
        print(x[0], x[1])


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print('usage: ./script JpnIHDS.dat <advanced>')
    else:
        if len(sys.argv) == 2:
            simple_parse(sys.argv[1])
        else:
            if sys.argv[2] == 'advanced':
                advanced_parse(sys.argv[1])
            else:
                print('Invalid argument:', sys.argv[2])
