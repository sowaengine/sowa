"""Regenerate independent ZIP reader fixtures using Python's standard library."""
from pathlib import Path
from zipfile import ZipFile, ZipInfo, ZIP_DEFLATED, ZIP_STORED, ZIP_BZIP2

root = Path(__file__).parent
with ZipFile(root / 'python.zip', 'w') as archive:
    for name, data, compression in [
        ('stored.bin', bytes(range(256)), ZIP_STORED),
        ('nested/deflated.txt', b'compressed asset\n' * 100, ZIP_DEFLATED),
        ('empty/', b'', ZIP_STORED),
        ('empty.txt', b'', ZIP_STORED),
        ('textures/oyuncu-\u00e7.txt', b'utf8', ZIP_DEFLATED),
        ('long/' + 'a' * 250 + '/' + 'b' * 250 + '/asset.txt', b'long name', ZIP_DEFLATED),
    ]:
        info = ZipInfo(name)
        info.compress_type = compression
        archive.writestr(info, data)
    with archive.open(ZipInfo('zip64.txt'), 'w', force_zip64=True) as out:
        out.write(b'zip64 local header')

for label, names in [
    ('traversal', ['../escape']),
    ('absolute', ['/absolute']),
    ('backslash', ['dir\\asset']),
    ('duplicate', ['same', './same']),
    ('collision', ['file/child', 'file']),
]:
    with ZipFile(root / (label + '.zip'), 'w') as archive:
        for name in names:
            archive.writestr(ZipInfo(name), b'payload')

# Damage stored file bytes without changing the central directory/CRC.
data = bytearray((root / 'python.zip').read_bytes())
offset = data.index(bytes(range(256)))
data[offset + 10] ^= 0xFF
(root / 'corrupt.zip').write_bytes(data)

with ZipFile(root / 'unsupported.zip', 'w') as archive:
    info = ZipInfo('bzip2.txt')
    info.compress_type = ZIP_BZIP2
    archive.writestr(info, b'unsupported compression')
