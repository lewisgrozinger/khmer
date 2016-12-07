/*
This file is part of khmer, https://github.com/dib-lab/khmer/, and is
Copyright (C) 2010-2015, Michigan State University.
Copyright (C) 2015-2016, The Regents of the University of California.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the Michigan State University nor the names
      of its contributors may be used to endorse or promote products
      derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
LICENSE (END)

Contact: khmer-project@idyll.org
*/
#ifndef HASHTABLE_HH
#define HASHTABLE_HH


#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

#include "khmer.hh"
#include "khmer_exception.hh"
#include "kmer_hash.hh"
#include "read_parsers.hh"
#include "storage.hh"
#include "subset.hh"

namespace khmer
{
namespace read_parsers
{
struct IParser;
}  // namespace read_parsers
}  // namespace khmer

#define CALLBACK_PERIOD 100000

namespace khmer
{
class Hashtable: public
    KmerFactory  		// Base class implementation of a Bloom ht.
{
    friend class SubsetPartition;
    friend class LabelHash;

protected:
    Storage * store;
    unsigned int    _max_count;
    unsigned int    _max_bigcount;

    //WordLength	    _ksize;
    HashIntoType    bitmask;
    unsigned int    _nbits_sub_1;

    explicit Hashtable( WordLength ksize, Storage * s)
        : KmerFactory( ksize ), store(s),
          _max_count( MAX_KCOUNT ),
          _max_bigcount( MAX_BIGCOUNT )
    {
        _init_bitstuff();
    }

    virtual ~Hashtable( )
    {
        delete store;
    }

    void _init_bitstuff()
    {
        bitmask = 0;
        for (unsigned int i = 0; i < _ksize; i++) {
            bitmask = (bitmask << 2) | 3;
        }
        _nbits_sub_1 = (_ksize*2 - 2);
    }

    explicit Hashtable(const Hashtable&);
    Hashtable& operator=(const Hashtable&);

public:
    // accessor to get 'k'
    const WordLength ksize() const
    {
        return _ksize;
    }

    // various hash functions.
    inline
    virtual
    HashIntoType
    hash_dna(const char * kmer) const
    {
        return _hash(kmer, _ksize);
    }

    inline
    virtual
    HashIntoType
    hash_dna_top_strand(const char * kmer) const
    {
        HashIntoType f = 0, r = 0;
        _hash(kmer, _ksize, f, r);
        return f;
    }

    inline
    virtual
    HashIntoType
    hash_dna_bottom_strand(const char * kmer) const
    {
        HashIntoType f = 0, r = 0;
        _hash(kmer, _ksize, f, r);
        return r;
    }

    inline
    virtual
    std::string
    unhash_dna(HashIntoType hashval) const
    {
        return _revhash(hashval, _ksize);
    }

    void count(const char * kmer)
    {
        store->add(hash_dna(kmer));
    }
    void count(HashIntoType khash)
    {
        store->add(khash);
    }
    void add(const char * kmer)
    {
        store->add(hash_dna(kmer));
    }
    void add(HashIntoType khash)
    {
        store->add(khash);
    }

    // get the count for the given k-mer.
    const BoundedCounterType get_count(const char * kmer) const
    {
        return store->get_count(hash_dna(kmer));
    }
    const BoundedCounterType get_count(HashIntoType khash) const
    {
        return store->get_count(khash);
    }

    void save(std::string filename)
    {
        store->save(filename, _ksize);
    }
    void load(std::string filename)
    {
        store->load(filename, _ksize);
        _init_bitstuff();
    }

    // count every k-mer in the string.
    unsigned int consume_string(const std::string &s);

    // checks each read for non-ACGT characters
    bool check_and_normalize_read(std::string &read) const;

    // check each read for non-ACGT characters, and then consume it.
    unsigned int check_and_process_read(std::string &read,
                                        bool &is_valid);

    // Count every k-mer in a FASTA or FASTQ file.
    // Note: Yes, the name 'consume_fasta' is a bit misleading,
    //	     but the FASTA format is effectively a subset of the FASTQ format
    //	     and the FASTA portion is what we care about in this case.
    void consume_fasta(
        std::string const   &filename,
        unsigned int	    &total_reads,
        unsigned long long  &n_consumed
    );

    // Count every k-mer from a stream of FASTA or FASTQ reads,
    // using the supplied parser.
    void consume_fasta(
        read_parsers:: IParser *	    parser,
        unsigned int	    &total_reads,
        unsigned long long  &n_consumed
    );

    void set_use_bigcount(bool b)
    {
        store->set_use_bigcount(b);
    }
    bool get_use_bigcount()
    {
        return store->get_use_bigcount();
    }

    bool median_at_least(const std::string &s,
                         unsigned int cutoff);

    void get_median_count(const std::string &s,
                          BoundedCounterType &median,
                          float &average,
                          float &stddev);

    // number of unique k-mers
    const uint64_t n_unique_kmers() const
    {
        return store->n_unique_kmers();
    }

    // count number of occupied bins
    const uint64_t n_occupied() const
    {
        return store->n_occupied();
    }

    // table information
    std::vector<uint64_t> get_tablesizes() const
    {
        return store->get_tablesizes();
    }
    const size_t n_tables() const
    {
        return store->n_tables();
    }

    // return all k-mer substrings, on the forward strand.
    void get_kmers(const std::string &s, std::vector<std::string> &kmers)
    const;

    // return hash values for all k-mer substrings
    void get_kmer_hashes(const std::string &s,
                         std::vector<HashIntoType> &kmers) const;

    // return hash values for all k-mer substrings in a SeenSet
    void get_kmer_hashes_as_hashset(const std::string &s,
                                    SeenSet& hashes) const;

    // return counts of all k-mers in this string.
    void get_kmer_counts(const std::string &s,
                         std::vector<BoundedCounterType> &counts) const;

    // get access to raw tables.
    Byte ** get_raw_tables()
    {
        return store->get_raw_tables();
    }

    // find the minimum k-mer count in the given sequence
    BoundedCounterType get_min_count(const std::string &s);

    // find the maximum k-mer count in the given sequence
    BoundedCounterType get_max_count(const std::string &s);

    // calculate the abundance distribution of kmers in the given file.
    uint64_t * abundance_distribution(read_parsers::IParser * parser,
                                      Hashtable * tracking);
    uint64_t * abundance_distribution(std::string filename,
                                      Hashtable * tracking);

    // return the index of the first position in the sequence with k-mer
    // abundance below min_abund.
    unsigned long trim_on_abundance(std::string seq,
                                    BoundedCounterType min_abund) const;

    // return the index of the first position in the sequence with k-mer
    // abundance above max_abund.
    unsigned long trim_below_abundance(std::string seq,
                                       BoundedCounterType max_abund) const;

    // detect likely positions of errors
    std::vector<unsigned int> find_spectral_error_positions(std::string seq,
            BoundedCounterType min_abund) const;
};

// Hashtable-derived class with ByteStorage.
class Counttable : public khmer::Hashtable
{
public:
    explicit Counttable(WordLength ksize, std::vector<uint64_t> sizes)
        : Hashtable(ksize, new ByteStorage(sizes)) { } ;
};

// Hashtable-derived class with NibbleStorage.
class SmallCounttable : public khmer::Hashtable
{
public:
    explicit SmallCounttable(WordLength ksize, std::vector<uint64_t> sizes)
        : Hashtable(ksize, new NibbleStorage(sizes)) { } ;
};

// Hashtable-derived class with BitStorage.
class Nodetable : public Hashtable
{
public:
    explicit Nodetable(WordLength ksize, std::vector<uint64_t> sizes)
        : Hashtable(ksize, new BitStorage(sizes)) { } ;
};

}

#endif // HASHTABLE_HH
