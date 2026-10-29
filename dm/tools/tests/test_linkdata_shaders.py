"""LINKDATA shaders are found through archive records, including compressed BIN/IDX pairs and the existing DATA0/DATA1 naming. No game files."""
import struct
import random
import tempfile
import unittest
import zlib
from pathlib import Path

from test_unity_text_glslc import glslc_output, ss


def chunked(data, size=0x10000):
    chunks = []
    for i in range(0, len(data), size):
        raw = data[i:i + size]; payload = zlib.compress(raw)
        chunks.append(raw if len(payload) + 4 >= len(raw) else struct.pack('<I', len(payload)) + payload)
    out = struct.pack('<III', size, len(chunks), len(data)) + struct.pack(f'<{len(chunks)}I', *map(len, chunks))
    out += bytes(-len(out) % 0x80)
    for chunk in chunks:
        out += chunk
        out += bytes(-len(out) % 0x80)
    return out


class LinkdataShaders(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)

    def archive(self, data_name='resources.BIN', index_name='resources.IDX'):
        shader = glslc_output()
        packed = chunked(shader)
        path = self.root / data_name
        path.write_bytes(packed + shader)
        index = self.root / index_name
        index.write_bytes(struct.pack('<QQQQ', 0, len(shader), len(packed), 1) +
                          struct.pack('<QQQQ', len(packed), len(shader), 0, 0) +
                          struct.pack('<QQQQ', 0xffffffffffffffff, 0, 0, 0))
        return str(path), index

    def test_compressed_and_stored_members(self):
        path, _ = self.archive()
        jobs = ss.linkdata_jobs(path, 'resources.BIN', parts=2)
        self.assertIsNotNone(jobs)
        found = []
        for p, rel, rng in jobs or []:
            ss.linkdata_shaders(p, rel, rng, found)
        expected = ss.glslc_shaders(glslc_output(), 'expected')
        self.assertEqual(len(found), 2)
        self.assertEqual([s[1:] for s in found], [expected[0][1:]] * 2)
        self.assertEqual([s[0] for s in found], ['/resources.BIN#0#GLSLC@0x0', '/resources.BIN#1#GLSLC@0x0'])

    def test_original_data_pair(self):
        path, _ = self.archive('DATA1.bin', 'DATA0.bin')
        self.assertEqual(len(ss.linkdata_entries(path) or []), 3)

    def test_variable_chunk_sizes(self):
        data = glslc_output()
        for size in (0x10000, 0x20000, 0x40000, 0x200000):
            with self.subTest(size=size):
                self.assertEqual(ss.kt_gz(chunked(data, size)), data)

    def test_mixed_chunks_and_short_last_chunk(self):
        data = b'x' * 4096 + random.Random(7).randbytes(4096) + b'z' * 77
        self.assertEqual(ss.kt_gz(chunked(data, 4096)), data)
        broken = bytearray(chunked(data, 4096))
        struct.pack_into('<I', broken, 0x80, len(broken))
        self.assertIsNone(ss.kt_gz(broken))

    def test_lowercase_index(self):
        path, _ = self.archive('other.bin', 'other.idx')
        self.assertEqual(len(ss.linkdata_entries(path) or []), 3)

    def test_index_bounds_and_layout(self):
        path, index = self.archive()
        for data in (b'bad', struct.pack('<QQQQ', 0, 1, 1, 2),
                     struct.pack('<QQQQ', 0, 1, 0, 1), struct.pack('<QQQQ', 2**63, 1, 1, 1),
                     struct.pack('<QQQQ', 0, 2**63, 0, 0)):
            with self.subTest(index=data):
                index.write_bytes(data)
                self.assertIsNone(ss.linkdata_jobs(path, 'resources.BIN'))

    def test_missing_index_is_a_loose_file(self):
        path, index = self.archive()
        index.unlink()
        self.assertIsNone(ss.linkdata_jobs(path, 'resources.BIN'))


if __name__ == '__main__':
    unittest.main()
