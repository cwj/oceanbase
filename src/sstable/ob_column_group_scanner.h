/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation. 
 *  
 * Version: 5567
 *
 * ob_column_group_scanner.h
 *
 * Authors:
 *     qushan <qushan@taobao.com>
 * Changes: 
 *     huating <huating.zmq@taobao.com>
 *
 */
#ifndef OCEANBASE_SSTABLE_COLUMN_GROUP_SCANNER_H_
#define OCEANBASE_SSTABLE_COLUMN_GROUP_SCANNER_H_

#include "common/ob_iterator.h"
#include "common/thread_buffer.h"
#include "ob_block_index_cache.h"
#include "ob_sstable_block_scanner.h"
#include "ob_sstable_scan_param.h"
#include "ob_scan_column_indexes.h"

namespace oceanbase
{
  namespace sstable
  {
    class ObSSTableReader;
    class ObBlockIndexCache;
    class ObBlockCache;

    class ObColumnGroupScanner : public common::ObIterator
    {
    public:
      static const int64_t UNCOMPRESSED_BLOCK_BUFSIZ = 256*1024;
      static const int64_t BLOCK_INTERNAL_BUFSIZ = 256*1024;
      ObColumnGroupScanner();
      virtual ~ObColumnGroupScanner();

      /**
       * implements interface of ObIterator, share same sematics.
       */
      int next_cell();
      int get_cell(oceanbase::common::ObCellInfo** cell);
      int get_cell(oceanbase::common::ObCellInfo** cell, bool* is_row_changed);

      /**
       * @param [in] group_id group id of this scanner.
       * @param [in] scan_param input scan param, query columns, table, etc.
       * @param [in] sstable_reader corresponds sstable
       *
       * @return 
       *  OB_SUCCESS  on success otherwise on failure.
       */
      int set_scan_param(const uint64_t group_id, const uint64_t group_seq, 
          const ObSSTableScanParam *scan_param, const ObSSTableReader* const sstable_reader);

      int initialize(ObBlockIndexCache* block_index_cache, ObBlockCache* block_cache);

    private:
      /**
       * check cursor if is legal.
       */
      inline bool is_valid_cursor() const
      {
        return ((index_array_cursor_>= 0)
                && (index_array_cursor_ < index_array_.block_count_)
                && (index_array_cursor_ != INVALID_CURSOR));
      } 

      inline bool is_forward_status() const
      {
        return iterate_status_ == ITERATE_IN_PROGRESS 
          || iterate_status_ == ITERATE_LAST_BLOCK
          || iterate_status_ == ITERATE_NEED_FORWARD;
      }

      int check_status() const;

      /**
       * is the end of block of current batch sstable blocks.
       * @return true if is the end of blocks.
       */
      bool is_end_of_block() const;

      /**
       * step to next block to scan;
       * in reverse mode, we scan block from end to start.
       * decrement cursor for every one step.
       */
      void advance_to_next_block();

      /**
       * load current block data into ObSSTableBlockScanner
       * but skip empty blocks which not contains data  in 
       * query range.
       */
      int fetch_next_block();

      /**
       * load current block data into ObSSTableBlockScanner
       * and advance iterate cursor to next block until stop.
       */
      int load_current_block_and_advance();


      /**
       * get block data from block cache, if not hit, read from disk.
       */
      int read_current_block_data(const char* &block_data_ptr, int64_t &block_data_size);

      
      /**
       * find block indexes in query range.
       */
      int search_block_index(const bool first_time);

      /**
       * translate column id into column index in sstable row.
       * it is useful when sstable represents dense data format.
       */
      int trans_input_column_id(const uint64_t group_id,
          const uint64_t group_seq,
          const ObSSTableScanParam *scan_param, 
          const ObSSTableReader *sstable_reader);

      /**
       * initialize for start to scan.
       */
      void reset_block_index_array();
      int prepare_read_blocks();

      /**
       * allocate memory from thread local arena.
       */
      int alloc_buffer(char* &buffer, const int64_t bufsiz);

    private:
      static const int32_t INVALID_CURSOR = 0xFFFFFFFF;
      static const int32_t TIME_OUT_US = 10 * 1000 * 1000;

      enum 
      {
        ITERATE_NOT_INITIALIZED = 1,
        ITERATE_NOT_START,
        ITERATE_IN_PROGRESS,
        ITERATE_LAST_BLOCK,
        ITERATE_NEED_FORWARD,
        ITERATE_IN_ERROR,
        ITERATE_END
      };
      
    private:
      // input parameters set by initialize 
      ObBlockIndexCache* block_index_cache_;
      ObBlockCache* block_cache_;

      // internal status
      int64_t iterate_status_;
      int64_t index_array_cursor_;
      ObBlockPositionInfos index_array_;

      // input parameters set by set_scan_param
      uint64_t group_id_;
      uint64_t group_seq_;
      const ObSSTableScanParam* scan_param_;
      const ObSSTableReader* sstable_reader_;

      char* uncompressed_data_buffer_;
      int64_t uncompressed_data_bufsiz_;
      char* block_internal_buffer_;
      int64_t block_internal_bufsiz_;

      // for densense format sstable
      ObScanColumnIndexes current_scan_column_indexes_;
      // scan in one block
      ObSSTableBlockScanner scanner_;
    };
  }//end namespace sstable
}//end namespace oceanbase

#endif
