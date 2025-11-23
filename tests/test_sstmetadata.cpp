#include "../LSM/SSTMetadata.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
#include <cstdio>

void test_sstmetadata_construction() {
    std::cout << "Testing SSTMetadata construction..." << std::endl;
    
    SSTMetadata sst("sst_1.txt", 10, 100, 45, 0, 1);
    
    assert(sst.fileName == "sst_1.txt");
    assert(sst.minKey == 10);
    assert(sst.maxKey == 100);
    assert(sst.numPairs == 45);
    assert(sst.level == 0);
    assert(sst.sstNumber == 1);
    
    std::cout << "✓ SSTMetadata construction test passed" << std::endl;
}

void test_might_contain() {
    std::cout << "Testing mightContain method..." << std::endl;
    
    SSTMetadata sst("sst_1.txt", 10, 100, 45, 0, 1);
    
    // Keys in range
    assert(sst.mightContain(10) == true);
    assert(sst.mightContain(50) == true);
    assert(sst.mightContain(100) == true);
    
    // Keys out of range
    assert(sst.mightContain(5) == false);
    assert(sst.mightContain(9) == false);
    assert(sst.mightContain(101) == false);
    assert(sst.mightContain(200) == false);
    
    std::cout << "✓ mightContain test passed" << std::endl;
}

void test_serialize_deserialize() {
    std::cout << "Testing serialize/deserialize..." << std::endl;
    
    SSTMetadata original("sst_42.txt", -100, 500, 1234, 2, 42);
    
    // Serialize
    std::string serialized = original.serialize();
    std::cout << "  Serialized: " << serialized << std::endl;
    
    // Deserialize
    SSTMetadata restored;
    bool success = restored.deserialize(serialized);
    
    assert(success == true);
    assert(restored.fileName == original.fileName);
    assert(restored.minKey == original.minKey);
    assert(restored.maxKey == original.maxKey);
    assert(restored.numPairs == original.numPairs);
    assert(restored.level == original.level);
    assert(restored.sstNumber == original.sstNumber);
    
    std::cout << "✓ Serialize/deserialize test passed" << std::endl;
}

void test_deserialize_invalid() {
    std::cout << "Testing deserialize with invalid input..." << std::endl;
    
    SSTMetadata sst;
    
    // Too few fields
    assert(sst.deserialize("1,2,file.txt") == false);
    
    // Too many fields
    assert(sst.deserialize("1,2,file.txt,10,20,30,40") == false);
    
    // Invalid number format
    assert(sst.deserialize("abc,2,file.txt,10,20,30") == false);
    
    std::cout << "✓ Invalid deserialize test passed" << std::endl;
}

void test_manifest_write_and_parse() {
    std::cout << "Testing manifest write and parse..." << std::endl;
    
    std::string testManifest = "test_manifest.txt";
    
    // Create test data
    std::vector<SSTMetadata> original;
    original.push_back(SSTMetadata("sst_1.txt", 10, 100, 45, 0, 1));
    original.push_back(SSTMetadata("sst_2.txt", 105, 200, 45, 0, 2));
    original.push_back(SSTMetadata("sst_3.txt", 5, 95, 90, 1, 3));
    
    int nextSST = 4;
    
    // Write manifest
    bool writeSuccess = ManifestUtils::writeManifest(testManifest, original, nextSST);
    assert(writeSuccess == true);
    
    // Parse manifest
    std::vector<SSTMetadata> parsed;
    int parsedNextSST = 0;
    bool parseSuccess = ManifestUtils::parseManifest(testManifest, parsed, parsedNextSST);
    
    assert(parseSuccess == true);
    assert(parsedNextSST == nextSST);
    assert(parsed.size() == original.size());
    
    // Verify each entry
    for (size_t i = 0; i < original.size(); i++) {
        assert(parsed[i].fileName == original[i].fileName);
        assert(parsed[i].minKey == original[i].minKey);
        assert(parsed[i].maxKey == original[i].maxKey);
        assert(parsed[i].numPairs == original[i].numPairs);
        assert(parsed[i].level == original[i].level);
        assert(parsed[i].sstNumber == original[i].sstNumber);
    }
    
    // Clean up
    std::remove(testManifest.c_str());
    
    std::cout << "✓ Manifest write and parse test passed" << std::endl;
}

void test_manifest_atomic_write() {
    std::cout << "Testing atomic manifest write..." << std::endl;
    
    std::string testManifest = "test_manifest_atomic.txt";
    
    // Create initial manifest
    std::vector<SSTMetadata> data1;
    data1.push_back(SSTMetadata("sst_1.txt", 1, 10, 5, 0, 1));
    ManifestUtils::writeManifest(testManifest, data1, 2);
    
    // Overwrite with new data
    std::vector<SSTMetadata> data2;
    data2.push_back(SSTMetadata("sst_2.txt", 11, 20, 5, 0, 2));
    data2.push_back(SSTMetadata("sst_3.txt", 21, 30, 5, 0, 3));
    ManifestUtils::writeManifest(testManifest, data2, 4);
    
    // Verify only new data exists
    std::vector<SSTMetadata> parsed;
    int nextSST;
    ManifestUtils::parseManifest(testManifest, parsed, nextSST);
    
    assert(parsed.size() == 2);
    assert(nextSST == 4);
    assert(parsed[0].fileName == "sst_2.txt");
    assert(parsed[1].fileName == "sst_3.txt");
    
    // Clean up
    std::remove(testManifest.c_str());
    
    std::cout << "✓ Atomic manifest write test passed" << std::endl;
}

void test_manifest_missing_file() {
    std::cout << "Testing manifest parse with missing file..." << std::endl;
    
    std::vector<SSTMetadata> parsed;
    int nextSST;
    bool success = ManifestUtils::parseManifest("nonexistent_manifest.txt", parsed, nextSST);
    
    assert(success == false);
    
    std::cout << "✓ Missing manifest file test passed" << std::endl;
}

void test_manifest_with_comments() {
    std::cout << "Testing manifest with comments and empty lines..." << std::endl;
    
    std::string testManifest = "test_manifest_comments.txt";
    
    // Create manifest with comments manually
    std::ofstream file(testManifest);
    file << "# This is a comment\n";
    file << "\n";
    file << "next_sst_number=5\n";
    file << "# Another comment\n";
    file << "0,1,sst_1.txt,10,100,45\n";
    file << "\n";
    file << "1,2,sst_2.txt,5,95,90\n";
    file.close();
    
    // Parse
    std::vector<SSTMetadata> parsed;
    int nextSST;
    bool success = ManifestUtils::parseManifest(testManifest, parsed, nextSST);
    
    assert(success == true);
    assert(nextSST == 5);
    assert(parsed.size() == 2);
    assert(parsed[0].sstNumber == 1);
    assert(parsed[1].sstNumber == 2);
    
    // Clean up
    std::remove(testManifest.c_str());
    
    std::cout << "✓ Manifest with comments test passed" << std::endl;
}

int main() {
    std::cout << "Running SSTMetadata tests..." << std::endl;
    std::cout << "================================" << std::endl;
    
    test_sstmetadata_construction();
    test_might_contain();
    test_serialize_deserialize();
    test_deserialize_invalid();
    test_manifest_write_and_parse();
    test_manifest_atomic_write();
    test_manifest_missing_file();
    test_manifest_with_comments();
    
    std::cout << "================================" << std::endl;
    std::cout << "All SSTMetadata tests passed! ✓" << std::endl;
    
    return 0;
}
