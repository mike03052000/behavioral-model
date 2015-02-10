#ifndef _BM_TABLES_H_
#define _BM_TABLES_H_

#include <unordered_map>
#include <string>

#include "entries.h"
#include "phv.h"
#include "actions.h"
#include "bytecontainer.h"
#include "handle_mgr.h"

using std::vector;
using std::unordered_map;
using std::pair;

typedef unsigned entry_handle_t;

struct MatchKeyBuilder
{
  vector< pair<header_id_t, int> > fields;

  void add_field(header_id_t header, int field_offset) {
    fields.push_back( pair<header_id_t, int>(header, field_offset) );
  }

  void operator()(const PHV &phv, ByteContainer &key) const
  {
    for(vector< pair<header_id_t, int> >::const_iterator it = fields.begin();
	it != fields.end();
	it++) {
      const Field &field = phv.get_field((*it).first, (*it).second);
      key.append(field.get_bytes());
    }
  }
};

class MatchTable
{
public:
  enum ErrorCode {
    SUCCESS = 0,
    TABLE_FULL,
    INVALID_HANDLE,
    ERROR
  };
public:
  MatchTable(size_t size, int nbytes_key,
	     const MatchKeyBuilder &match_key_builder)
    : size(size), num_entries(0),
      nbytes_key(nbytes_key),
      match_key_builder(match_key_builder) { }
  
  void apply(const PHV &phv);
  
  virtual const MatchEntry *lookup(const ByteContainer &key) const = 0;
  virtual ErrorCode delete_entry(entry_handle_t handle);

  size_t get_num_entries() const {return num_entries;}

protected:
  size_t size;
  size_t num_entries;
  int nbytes_key;
  HandleMgr handles;
  
  MatchKeyBuilder match_key_builder;
  void build_key(const PHV &phv, ByteContainer &key) {
    match_key_builder(phv, key);
  }
  
  ErrorCode get_and_set_handle(entry_handle_t *handle);
  ErrorCode unset_handle(entry_handle_t handle);

  bool valid_handle(entry_handle_t handle) const;
};

class ExactMatchTable : public MatchTable
{
public:
  ExactMatchTable(int size, int nbytes_key,
		  const MatchKeyBuilder &match_key_builder)
    : MatchTable(size, nbytes_key, match_key_builder) {
    entries = vector<ExactMatchEntry>(size);
  }

  const ExactMatchEntry *lookup(const ByteContainer &key) const;
  ErrorCode add_entry(const ByteContainer &key, const ActionFn &action_fn,
		      entry_handle_t *handle);
  ErrorCode delete_entry(entry_handle_t handle);

private:
  vector<ExactMatchEntry> entries;
  unordered_map<ByteContainer, int, ByteContainerKeyHash> entries_map;
};

class LongestPrefixMatchTable : public MatchTable
{
public:
  LongestPrefixMatchTable(int size, int nbytes_key, 
			  const MatchKeyBuilder &match_key_builder)
    : MatchTable(size, nbytes_key, match_key_builder) {
    entries = vector<LongestPrefixMatchEntry>(size);
  }
  
  const LongestPrefixMatchEntry *lookup(const ByteContainer &key) const;
  ErrorCode add_entry(const ByteContainer &key, int prefix_length,
		      const ActionFn &action_fn, entry_handle_t *handle);
  ErrorCode delete_entry(entry_handle_t handle);

private:
  vector<LongestPrefixMatchEntry> entries;
};


class TernaryMatchTable : public MatchTable
{
public:
  TernaryMatchTable(int size, int nbytes_key,
		    const MatchKeyBuilder &match_key_builder)
    : MatchTable(size, nbytes_key, match_key_builder) {
    entries = vector<TernaryMatchEntry>(size);
  }

  const TernaryMatchEntry *lookup(const ByteContainer &key) const;
  ErrorCode add_entry(const ByteContainer &key, const ByteContainer &mask,
		      int priority, const ActionFn &action_fn, entry_handle_t *handle);
  ErrorCode delete_entry(entry_handle_t handle);

private:
  vector<TernaryMatchEntry> entries;
};

#endif