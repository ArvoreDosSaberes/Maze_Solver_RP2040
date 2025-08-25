/**
 * @file PersistentMemory.cpp
 * @brief Implementação da persistência em flash (RP2040) e em arquivo (host).
 */
#include "PersistentMemory.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#if !defined(PICO_BUILD)
#  include <filesystem>
#  include <fstream>
#endif

namespace maze {

// -----------------------------
// Common in-memory fallback
/**
 * @brief Últimas heurísticas salvas (fallback em memória para host e RP2040).
 */
static Heuristics g_last_heuristics{};
/**
 * @brief Indica se há heurísticas válidas no fallback em memória.
 */
static bool g_has_heuristics = false;

#ifdef PICO_BUILD
#  include "pico/stdlib.h"
#  include "hardware/flash.h"
#  include "hardware/sync.h"

/**
 * @brief Tamanho do setor de flash reservado para persistência (bytes).
 */
static constexpr uint32_t SECTOR_SIZE = 4096u;
/**
 * @brief Tamanho da página de programação da flash (bytes).
 */
static constexpr uint32_t PAGE_SIZE   = 256u;
// Compute an address near end of flash. The XIP_BASE is where code is mapped.
// We'll store at (XIP_BASE + FLASH_TOTAL_SIZE - SECTOR_SIZE). FLASH_TOTAL_SIZE is not exposed
// portably at compile-time here, so we assume default 2MB flash and tolerate larger parts.
// If using boards with different sizes, define PMEM_FLASH_TOTAL_BYTES via CMake.
/**
 * @def PMEM_FLASH_TOTAL_BYTES
 * @brief Tamanho total da flash (bytes) assumido para calcular o offset do setor reservado.
 *
 * Pode ser definido via CMake para boards com capacidades diferentes.
 */
#ifndef PMEM_FLASH_TOTAL_BYTES
#define PMEM_FLASH_TOTAL_BYTES (2u * 1024u * 1024u)
#endif
/**
 * @brief Offset do setor reservado a partir do início da flash (bytes).
 */
static constexpr uint32_t FLASH_TARGET_OFFSET = PMEM_FLASH_TOTAL_BYTES - SECTOR_SIZE; // from start of flash

/**
 * @brief Cabeçalho do registro de heurísticas na primeira página do setor.
 */
struct __attribute__((packed)) FlashRecordHeader {
    uint32_t magic;     ///< Assinatura mágica para validar conteúdo
    uint16_t version;   ///< Versão do layout dos dados
    uint16_t size;      ///< Tamanho do payload (sizeof(Heuristics))
};

/** @brief Magic para heurísticas ('M','Z','H','U'). */
static constexpr uint32_t REC_MAGIC = 0x4D5A4855u; // 'MZHU'
/** @brief Versão do registro de heurísticas. */
static constexpr uint16_t REC_VER   = 0x0001u;

/**
 * @brief Cabeçalho do snapshot do mapa (segunda página do setor).
 */
struct __attribute__((packed)) MapHeader {
    uint32_t magic;   ///< 'M','Z','M','P'
    uint16_t version; ///< Versão do snapshot (0x0001)
    uint16_t w;       ///< Largura
    uint16_t h;       ///< Altura
    uint16_t size;    ///< Tamanho do payload em bytes (w*h)
};
/** @brief Magic para mapa ('M','Z','M','P'). */
static constexpr uint32_t MAP_MAGIC = 0x4D5A4D50u; // 'MZMP'
/** @brief Versão do snapshot de mapa. */
static constexpr uint16_t MAP_VER   = 0x0001u;

/**
 * @brief Ponteiro base para o setor reservado na XIP flash.
 */
static const uint8_t* flash_ptr() {
    return reinterpret_cast<const uint8_t*>(XIP_BASE + FLASH_TARGET_OFFSET);
}

// Close PICO-only block here so following helpers/functions are compiled on host too
#endif // PICO_BUILD

// -----------------------------
// Map snapshot encode/decode helpers (host and pico)
/**
 * @brief Codifica o mapa em bytes (4 bits utilizados: NESW).
 * @param map mapa de entrada
 * @param bytes vetor de saída com w*h bytes
 */
static void pmem_encode_map_bytes(const MazeMap& map, std::vector<uint8_t>& bytes) {
    const int w = map.width();
    const int h = map.height();
    bytes.resize(static_cast<size_t>(w*h));
    for (int y=0; y<h; ++y) {
        for (int x=0; x<w; ++x) {
            const Cell& c = map.at(x,y);
            uint8_t b = 0;
            if (c.wall_n) b |= 1u;
            if (c.wall_e) b |= 2u;
            if (c.wall_s) b |= 4u;
            if (c.wall_w) b |= 8u;
            bytes[static_cast<size_t>(y*w + x)] = b;
        }
    }
}

/**
 * @brief Decodifica bytes (NESW) para paredes no `MazeMap` de saída.
 * @param out mapa já alocado (dimensões usadas para validar)
 * @param data ponteiro para buffer de entrada
 * @param len comprimento do buffer
 */
static void pmem_decode_map_bytes(MazeMap* out, const uint8_t* data, int len) {
    const int w = out->width();
    const int h = out->height();
    if (len < w*h) return;
    for (int y=0; y<h; ++y) {
        for (int x=0; x<w; ++x) {
            uint8_t b = data[static_cast<size_t>(y*w + x)];
            if (b & 1u) out->set_wall(x,y,'N',true);
            if (b & 2u) out->set_wall(x,y,'E',true);
            if (b & 4u) out->set_wall(x,y,'S',true);
            if (b & 8u) out->set_wall(x,y,'W',true);
        }
    }
}

/** @copydoc PersistentMemory::saveMapSnapshot */
bool PersistentMemory::saveMapSnapshot(const MazeMap& map) {
#ifdef PICO_BUILD
    // Map header definition (only visible under PICO_BUILD)
    struct __attribute__((packed)) MapHeader {
        uint32_t magic;   // 'MZMP'
        uint16_t version; // 0x0001
        uint16_t w;       // width
        uint16_t h;       // height
        uint16_t size;    // payload size bytes
    };
    static constexpr uint32_t MAP_MAGIC = 0x4D5A4D50u; // 'MZMP'
    static constexpr uint16_t MAP_VER   = 0x0001u;

    const int w = map.width();
    const int h = map.height();
    std::vector<uint8_t> bytes;
    pmem_encode_map_bytes(map, bytes);
    const uint16_t payload = static_cast<uint16_t>(bytes.size());
    if (sizeof(MapHeader) + payload > PAGE_SIZE) {
        std::printf("PMEM[PICO]: saveMapSnapshot too large (%u > %u)\n", (unsigned)(sizeof(MapHeader)+payload), (unsigned)PAGE_SIZE);
        return false;
    }
    alignas(4) uint8_t page2[PAGE_SIZE];
    std::memset(page2, 0xFF, sizeof(page2));
    MapHeader mh{MAP_MAGIC, MAP_VER, static_cast<uint16_t>(w), static_cast<uint16_t>(h), payload};
    std::memcpy(page2, &mh, sizeof(mh));
    std::memcpy(page2 + sizeof(mh), bytes.data(), payload);
    uint32_t ints = save_and_disable_interrupts();
    // Program second page within the reserved sector (first page holds heuristics)
    flash_range_program(FLASH_TARGET_OFFSET + PAGE_SIZE, page2, PAGE_SIZE);
    restore_interrupts(ints);
    std::printf("PMEM[PICO]: saveMapSnapshot ok (%dx%d)\n", w, h);
    return true;
#else
    const char* home = std::getenv("HOME");
    if (!home) return false;
    std::filesystem::path dir = std::filesystem::path(home) / ".rp2040_maze";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) return false;
    std::filesystem::path file = dir / "map.bin";
    std::ofstream ofs(file, std::ios::binary | std::ios::trunc);
    if (!ofs) return false;
    std::vector<uint8_t> bytes;
    pmem_encode_map_bytes(map, bytes);
    struct MapHeaderHost { uint32_t magic; uint16_t version,w,h,size; };
    MapHeaderHost mh{0x4D5A4D50u, 0x0001u, static_cast<uint16_t>(map.width()), static_cast<uint16_t>(map.height()), static_cast<uint16_t>(bytes.size())};
    ofs.write(reinterpret_cast<const char*>(&mh), sizeof(mh));
    ofs.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    ofs.close();
    std::printf("PMEM[HOST]: saveMapSnapshot ok -> %s\n", file.string().c_str());
    return true;
#endif
}

/** @copydoc PersistentMemory::loadMapSnapshot */
bool PersistentMemory::loadMapSnapshot(MazeMap* out) {
    if (!out) return false;
#ifdef PICO_BUILD
    struct __attribute__((packed)) MapHeader { uint32_t magic; uint16_t version,w,h,size; };
    static constexpr uint32_t MAP_MAGIC = 0x4D5A4D50u; // 'MZMP'
    static constexpr uint16_t MAP_VER   = 0x0001u;
    const uint8_t* base = flash_ptr();
    MapHeader mh{};
    std::memcpy(&mh, base + PAGE_SIZE, sizeof(mh));
    if (!(mh.magic == MAP_MAGIC && mh.version == MAP_VER)) return false;
    if (mh.w != out->width() || mh.h != out->height()) return false;
    if (mh.size > PAGE_SIZE - sizeof(MapHeader)) return false;
    pmem_decode_map_bytes(out, base + PAGE_SIZE + sizeof(MapHeader), mh.size);
    std::printf("PMEM[PICO]: loadMapSnapshot ok (%ux%u)\n", mh.w, mh.h);
    return true;
#else
    const char* home = std::getenv("HOME");
    if (!home) return false;
    std::filesystem::path file = std::filesystem::path(home) / ".rp2040_maze" / "map.bin";
    std::ifstream ifs(file, std::ios::binary);
    if (!ifs) return false;
    struct MapHeaderHost { uint32_t magic; uint16_t version,w,h,size; } mh{};
    ifs.read(reinterpret_cast<char*>(&mh), sizeof(mh));
    if (!(mh.magic == 0x4D5A4D50u && mh.version == 0x0001u)) return false;
    if (mh.w != out->width() || mh.h != out->height()) return false;
    std::vector<uint8_t> bytes(mh.size);
    ifs.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (ifs.gcount() != static_cast<std::streamsize>(bytes.size())) return false;
    pmem_decode_map_bytes(out, bytes.data(), static_cast<int>(bytes.size()));
    std::printf("PMEM[HOST]: loadMapSnapshot ok\n");
    return true;
#endif
}

/**
 * @brief Verifica se há registro de heurísticas válido na flash (RP2040).
 */
#ifdef PICO_BUILD
static bool flash_has_valid_record() {
    const auto* p = flash_ptr();
    FlashRecordHeader hdr;
    std::memcpy(&hdr, p, sizeof(hdr));
    return hdr.magic == REC_MAGIC && hdr.version == REC_VER && hdr.size == sizeof(Heuristics);
}
#endif // PICO_BUILD

/** @copydoc PersistentMemory::eraseAll */
bool PersistentMemory::eraseAll() {
#ifdef PICO_BUILD
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, SECTOR_SIZE);
    restore_interrupts(ints);
    g_has_heuristics = false;
    std::printf("PMEM[PICO]: eraseAll() ok\n");
    return true;
#else
    const char* home = std::getenv("HOME");
    if (!home) return false;
    std::filesystem::path dir = std::filesystem::path(home) / ".rp2040_maze";
    std::filesystem::path file_h = dir / "heuristics.bin";
    std::filesystem::path file_m = dir / "map.bin";
    std::error_code ec1, ec2;
    bool r1 = std::filesystem::remove(file_h, ec1);
    bool r2 = std::filesystem::remove(file_m, ec2);
    std::printf("PMEM[HOST]: eraseAll() heur=%s map=%s\n", (r1 && !ec1) ? "ok" : "noop", (r2 && !ec2) ? "ok" : "noop");
    return ((r1 && !ec1) || !std::filesystem::exists(file_h)) && ((r2 && !ec2) || !std::filesystem::exists(file_m));
#endif
}

/** @copydoc PersistentMemory::status */
PersistenceStatus PersistentMemory::status() {
#ifdef PICO_BUILD
    PersistenceStatus st{};
    st.saved_count = flash_has_valid_record() ? 1u : 0u;
    st.active_profile = 0u;
    return st;
#else
    PersistenceStatus st{};
    const char* home = std::getenv("HOME");
    if (!home) return st;
    std::filesystem::path dir = std::filesystem::path(home) / ".rp2040_maze";
    std::filesystem::path file = dir / "heuristics.bin";
    if (std::filesystem::exists(file)) st.saved_count = 1;
    st.active_profile = 0;
    return st;
#endif
}

/** @copydoc PersistentMemory::saveHeuristics */
bool PersistentMemory::saveHeuristics(const Heuristics& h) {
    g_last_heuristics = h;
    g_has_heuristics = true;
#ifdef PICO_BUILD
    // Prepare page-aligned buffer: header + payload
    alignas(4) uint8_t page[PAGE_SIZE] = {0xFF};
    FlashRecordHeader hdr{REC_MAGIC, REC_VER, static_cast<uint16_t>(sizeof(Heuristics))};
    std::memcpy(page, &hdr, sizeof(hdr));
    std::memcpy(page + sizeof(hdr), &h, sizeof(Heuristics));
    // Erase sector, then program first page (sufficient for our data size)
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, page, PAGE_SIZE);
    restore_interrupts(ints);
    std::printf("PMEM[PICO]: saveHeuristics ok (r=%.2f f=%.2f l=%.2f b=%.2f)\n", h.w_right, h.w_front, h.w_left, h.w_back);
    return true;
#else
    const char* home = std::getenv("HOME");
    if (!home) {
        std::printf("PMEM[HOST]: HOME not set, keeping in-memory only\n");
        return true;
    }
    std::filesystem::path dir = std::filesystem::path(home) / ".rp2040_maze";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        std::printf("PMEM[HOST]: create dir failed: %s\n", ec.message().c_str());
        return false;
    }
    std::filesystem::path file = dir / "heuristics.bin";
    std::ofstream ofs(file, std::ios::binary | std::ios::trunc);
    if (!ofs) {
        std::printf("PMEM[HOST]: open write failed\n");
        return false;
    }
    ofs.write(reinterpret_cast<const char*>(&h), sizeof(Heuristics));
    ofs.close();
    std::printf("PMEM[HOST]: saveHeuristics ok -> %s\n", file.string().c_str());
    return true;
#endif
}

/** @copydoc PersistentMemory::loadHeuristics */
bool PersistentMemory::loadHeuristics(Heuristics* out) {
    if (!out) return false;
#ifdef PICO_BUILD
    if (flash_has_valid_record()) {
        Heuristics tmp{};
        const uint8_t* p = flash_ptr();
        tmp = {};
        std::memcpy(&tmp, p + sizeof(FlashRecordHeader), sizeof(Heuristics));
        *out = tmp;
        g_last_heuristics = tmp;
        g_has_heuristics = true;
        std::printf("PMEM[PICO]: loadHeuristics ok (r=%.2f f=%.2f l=%.2f b=%.2f)\n", out->w_right, out->w_front, out->w_left, out->w_back);
        return true;
    }
    if (!g_has_heuristics) return false;
    *out = g_last_heuristics;
    std::printf("PMEM[PICO]: loadHeuristics from RAM fallback\n");
    return true;
#else
    const char* home = std::getenv("HOME");
    if (home) {
        std::filesystem::path file = std::filesystem::path(home) / ".rp2040_maze" / "heuristics.bin";
        std::ifstream ifs(file, std::ios::binary);
        if (ifs) {
            Heuristics tmp{};
            ifs.read(reinterpret_cast<char*>(&tmp), sizeof(Heuristics));
            if (ifs.gcount() == static_cast<std::streamsize>(sizeof(Heuristics))) {
                *out = tmp;
                g_last_heuristics = tmp;
                g_has_heuristics = true;
                std::printf("PMEM[HOST]: loadHeuristics ok\n");
                return true;
            }
        }
    }
    if (!g_has_heuristics) return false;
    *out = g_last_heuristics;
    std::printf("PMEM[HOST]: loadHeuristics from memory\n");
    return true;
#endif
}

} // namespace maze

