/****************************************************************************
 * Driver for Solarflare network controllers and boards
 * Copyright 2019 Solarflare Communications Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation, incorporated herein by reference.
 */

#ifndef EFX_RX_COMMON_H
#define EFX_RX_COMMON_H
#include "mcdi_pcol.h"
#include "mcdi.h"
#include "net_driver.h"

void efx_rx_config_page_split(struct efx_nic *efx);

int efx_probe_rx_queue(struct efx_rx_queue *rx_queue);
int efx_init_rx_queue(struct efx_rx_queue *rx_queue);
void efx_fini_rx_queue(struct efx_rx_queue *rx_queue);
void efx_remove_rx_queue(struct efx_rx_queue *rx_queue);
void efx_destroy_rx_queue(struct efx_rx_queue *rx_queue);

void efx_init_rx_buffer(struct efx_rx_queue *rx_queue,
				struct page *page,
				unsigned int page_offset,
				u16 flags);

void efx_unmap_rx_buffer(struct efx_nic *efx, struct efx_rx_buffer *rx_buf);

static inline void efx_sync_rx_buffer(struct efx_nic *efx,
				      struct efx_rx_buffer *rx_buf,
				      unsigned int len)
{
#if !defined(EFX_NOT_UPSTREAM) || defined(EFX_RX_PAGE_SHARE)
	dma_sync_single_for_cpu(&efx->pci_dev->dev, rx_buf->dma_addr, len,
				DMA_FROM_DEVICE);
#else
	efx_unmap_rx_buffer(efx, rx_buf);
#endif
}

void efx_free_rx_buffers(struct efx_rx_queue *rx_queue,
			 struct efx_rx_buffer *rx_buf,
			 unsigned int num_bufs);

#if !defined(EFX_USE_KCOMPAT) || defined(EFX_HAVE_XDP_SOCK)
void efx_recycle_rx_bufs_zc(struct efx_channel *channel,
			    struct efx_rx_buffer *rx_buf,
			    unsigned int n_frags);
#endif

void efx_recycle_rx_pages(struct efx_channel *channel,
			  struct efx_rx_buffer *rx_buf,
			  unsigned int n_frags);

void efx_discard_rx_packet(struct efx_channel *channel,
			   struct efx_rx_buffer *rx_buf,
			   unsigned int n_frags);

static inline u8 *efx_rx_buf_va(struct efx_rx_buffer *buf)
{
#if !defined(EFX_USE_KCOMPAT) || defined(EFX_HAVE_XDP_SOCK)
	if (buf->flags & EFX_RX_BUF_FROM_UMEM)
#if !defined(EFX_USE_KCOMPAT) || defined(EFX_USE_XSK_BUFFER_ALLOC)
		return ((u8 *)buf->xsk_buf->data + buf->page_offset);
#else
		return ((u8 *)buf->addr + buf->page_offset);
#endif
#endif
	return page_address(buf->page) + buf->page_offset;
}

bool efx_rx_buf_hash_valid(u8 *prefix);

static inline u32 efx_rx_buf_hash(struct efx_nic *efx, const u8 *eh)
{
#if defined(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS)
	return __le32_to_cpup((const __le32 *)
			      (eh + efx->rx_packet_hash_offset));
#else
	const u8 *data = eh + efx->rx_packet_hash_offset;
	return (u32)data[0]       |
	       (u32)data[1] << 8  |
	       (u32)data[2] << 16 |
	       (u32)data[3] << 24;
#endif
}

void efx_schedule_slow_fill(struct efx_rx_queue *rx_queue);
void efx_cancel_slow_fill(struct efx_rx_queue *rx_queue);
void efx_fast_push_rx_descriptors(struct efx_rx_queue *rx_queue, bool atomic);

void
efx_rx_packet_gro(struct efx_channel *channel, struct efx_rx_buffer *rx_buf,
		  unsigned int n_frags, u8 *eh, __wsum csum);

bool efx_filter_is_mc_recipient(const struct efx_filter_spec *spec);
bool efx_filter_spec_equal(const struct efx_filter_spec *left,
			   const struct efx_filter_spec *right);
u32 efx_filter_spec_hash(const struct efx_filter_spec *spec);

#ifdef CONFIG_RFS_ACCEL
bool efx_rps_check_rule(struct efx_arfs_rule *rule, unsigned int filter_idx,
			bool *force);

struct efx_arfs_rule *efx_rps_hash_find(struct efx_nic *efx,
					const struct efx_filter_spec *spec);

/* @new is written to indicate if entry was newly added (true) or if an old
 * entry was found and returned (false).
 */
struct efx_arfs_rule *efx_rps_hash_add(struct efx_nic *efx,
				       const struct efx_filter_spec *spec,
				       bool *new);

void efx_rps_hash_del(struct efx_nic *efx, const struct efx_filter_spec *spec);
#endif

int efx_probe_filters(struct efx_nic *efx);
void efx_remove_filters(struct efx_nic *efx);

static inline bool efx_tx_vi_spreading(struct efx_nic *efx)
{
	return efx->mcdi->fn_flags &
	       (1 << MC_CMD_DRV_ATTACH_EXT_OUT_FLAG_TX_ONLY_VI_SPREADING_ENABLED);
}
int efx_rx_queue_id_internal(struct efx_nic *efx, int rxq_id);
static inline int efx_rx_queue_instance(struct efx_rx_queue *rxq)
{
	return efx_rx_queue_id_internal(rxq->efx, efx_rx_queue_index(rxq));
}

#endif

