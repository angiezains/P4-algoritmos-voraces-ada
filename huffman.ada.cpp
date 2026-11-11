
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <queue>
#include <stdexcept>
#include <string>
#include <vector>

struct HuffmanNode {
    uint8_t character;
    uint64_t frequency;
    std::shared_ptr<HuffmanNode> left, right;
    HuffmanNode(uint8_t ch, uint64_t f) : character(ch), frequency(f) {}
    HuffmanNode(std::shared_ptr<HuffmanNode> l, std::shared_ptr<HuffmanNode> r)
        : character(0), frequency(l->frequency + r->frequency), left(l), right(r) {}
    bool is_leaf() const { return !left && !right; }
};
using NodePtr = std::shared_ptr<HuffmanNode>;

struct NodeComparator {
    bool operator()(const NodePtr& a, const NodePtr& b) const {
        return a->frequency > b->frequency; 
};


NodePtr build_huffman_tree(const std::vector<uint64_t>& freq) {
    std::priority_queue<NodePtr, std::vector<NodePtr>, NodeComparator> heap;
    for (int i = 0; i < 256; ++i)
        if (freq[i] > 0) heap.push(std::make_shared<HuffmanNode>((uint8_t)i, freq[i]));
    if (heap.empty()) return nullptr;
    if (heap.size() == 1) {
        auto only = heap.top();
        auto dummy = std::make_shared<HuffmanNode>((uint8_t)(only->character + 1), 0);
        return std::make_shared<HuffmanNode>(only, dummy);
    }
    while (heap.size() > 1) {
        auto a = heap.top(); heap.pop();
        auto b = heap.top(); heap.pop();
        heap.push(std::make_shared<HuffmanNode>(a, b));
    }
    return heap.top();
}

class BitWriter {
    std::vector<uint8_t> buf_;
    uint8_t cur_ = 0;
    int nbits_ = 0;  
public:
    void put_bit(unsigned b) {
        cur_ = (uint8_t)((cur_ << 1) | (b & 1u));
        if (++nbits_ == 8) { buf_.push_back(cur_); cur_ = 0; nbits_ = 0; }
    }
    void put_bits(uint64_t v, int n) {  
        for (int i = n - 1; i >= 0; --i) put_bit((unsigned)((v >> i) & 1u));
    }
    void flush() {  
        if (nbits_) { buf_.push_back((uint8_t)(cur_ << (8 - nbits_))); cur_ = 0; nbits_ = 0; }
    }
    const std::vector<uint8_t>& bytes() const { return buf_; }
};

class BitReader {
    const std::vector<uint8_t>& buf_;
    size_t pos_ = 0;  
public:
    explicit BitReader(const std::vector<uint8_t>& b) : buf_(b) {}
    unsigned get_bit() {
        if (pos_ >= buf_.size() * 8) throw std::runtime_error("fin de datos inesperado");
        unsigned b = (buf_[pos_ >> 3] >> (7 - (pos_ & 7))) & 1u;
        ++pos_;
        return b;
    }
    uint64_t get_bits(int n) {
        uint64_t v = 0;
        for (int i = 0; i < n; ++i) v = (v << 1) | get_bit();
        return v;
    }
    bool eof() const { return pos_ >= buf_.size() * 8; }
};

void write_tree(const NodePtr& n, BitWriter& bw) {
    if (n->is_leaf()) { bw.put_bit(1); bw.put_bits(n->character, 8); }
    else { bw.put_bit(0); write_tree(n->left, bw); write_tree(n->right, bw); }
}
NodePtr read_tree(BitReader& br, int depth = 0) {
    if (depth > 300) throw std::runtime_error("árbol corrupto");
    if (br.get_bit()) return std::make_shared<HuffmanNode>((uint8_t)br.get_bits(8), 0);
    auto l = read_tree(br, depth + 1);
    auto r = read_tree(br, depth + 1);
    return std::make_shared<HuffmanNode>(l, r);
}

void fill_codes(const NodePtr& n, std::vector<uint8_t>& path, std::vector<std::vector<uint8_t>>& codes) {
    if (n->is_leaf()) { codes[n->character] = path; return; }
    path.push_back(0); fill_codes(n->left, path, codes); path.pop_back();
    path.push_back(1); fill_codes(n->right, path, codes); path.pop_back();
}


std::vector<uint8_t> compress(const std::vector<uint8_t>& in) {
    BitWriter bw;
    for (char c : std::string("HUF1")) bw.put_bits((uint8_t)c, 8);
    bw.put_bits(in.size(), 64);
    if (in.empty()) { bw.flush(); return bw.bytes(); }

    std::vector<uint64_t> freq(256, 0);
    for (uint8_t b : in) ++freq[b];
    auto root = build_huffman_tree(freq);
    write_tree(root, bw);

    std::vector<std::vector<uint8_t>> codes(256);
    std::vector<uint8_t> path;
    fill_codes(root, path, codes);
    for (uint8_t b : in)
        for (uint8_t bit : codes[b]) bw.put_bit(bit);
    bw.flush();
    return bw.bytes();
}

std::vector<uint8_t> decompress(const std::vector<uint8_t>& in) {
    BitReader br(in);
    std::string magic;
    for (int i = 0; i < 4; ++i) magic += (char)br.get_bits(8);
    if (magic != "HUF1") throw std::runtime_error("formato inválido");
    uint64_t n = br.get_bits(64);
    std::vector<uint8_t> out;
    if (n == 0) return out;
    out.reserve(n);
    auto root = read_tree(br);
    for (uint64_t i = 0; i < n; ++i) {  
        auto node = root;
        while (!node->is_leaf()) node = br.get_bit() ? node->right : node->left;
        out.push_back(node->character);
    }
    return out;
}


std::vector<uint8_t> decompress_naive(const std::vector<uint8_t>& in) {
    BitReader br(in);
    for (int i = 0; i < 4; ++i) br.get_bits(8);
    br.get_bits(64);
    auto root = read_tree(br);
    std::vector<uint8_t> out;
    try {
        while (!br.eof()) {
            auto node = root;
            while (!node->is_leaf()) node = br.get_bit() ? node->right : node->left;
            out.push_back(node->character);
        }
    } catch (...) {}  
    return out;
}


std::vector<uint8_t> read_file(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) throw std::runtime_error("no se pudo abrir " + p);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(f), {});
}
void write_file(const std::string& p, const std::vector<uint8_t>& d) {
    std::ofstream f(p, std::ios::binary);
    f.write((const char*)d.data(), (std::streamsize)d.size());
}

int main(int argc, char** argv) {
    try {
        std::string mode = argc > 1 ? argv[1] : "";
        if (mode == "selftest") {
            std::string m = "ABRACADABRA_ALGORITMO_AVANZADO_ESCOM";
            std::vector<uint8_t> in(m.begin(), m.end());
            auto c = compress(in);
            auto good = decompress(c);
            auto bad = decompress_naive(c);
            std::cout << "original=" << in.size() << " B, comprimido=" << c.size() << " B\n";
            std::cout << "decoder con N : " << std::string(good.begin(), good.end()) << "\n";
            std::cout << "decoder ingenuo: " << std::string(bad.begin(), bad.end()) << "  (" << bad.size() << " chars)\n";
            return good == in ? 0 : 1;
        }
        if (argc != 4 || (mode != "c" && mode != "d")) {
            std::cerr << "uso: " << argv[0] << " c|d entrada salida | selftest\n";
            return 2;
        }
        auto in = read_file(argv[2]);
        auto out = mode == "c" ? compress(in) : decompress(in);
        write_file(argv[3], out);
        if (mode == "c")
            std::cout << "original=" << in.size() << " B  comprimido=" << out.size() << " B  ratio="
                      << (in.empty() ? 0.0 : 100.0 * out.size() / in.size()) << "%\n";
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
