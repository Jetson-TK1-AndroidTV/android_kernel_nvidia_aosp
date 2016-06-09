/*
 * tegra30_offload.c - Tegra platform driver for offload rendering
 *
 * Author: Sumit Bhattacharya <sumitb@nvidia.com>
 * Copyright (c) 2014, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

/*
#define VERBOSE_DEBUG 1
#define DEBUG 1
*/

#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/compress_driver.h>
#include <sound/dmaengine_pcm.h>

#include "tegra_pcm.h"
#include "tegra_offload.h"

#define DRV_NAME "tegra-offload"

enum {
	PCM_OFFLOAD_DAI,
	COMPR_OFFLOAD_DAI,
	MAX_OFFLOAD_DAI
};

struct tegra_offload_pcm_data {
	struct tegra_offload_pcm_ops *ops;
	int appl_ptr;
	int stream_id;
};

struct tegra_offload_compr_data {
	struct tegra_offload_compr_ops *ops;
	struct snd_codec codec;
	int stream_id;
};

static struct tegra_offload_ops offload_ops;
static int tegra_offload_init_done;
static DEFINE_MUTEX(tegra_offload_lock);

static const struct snd_pcm_hardware tegra_offload_pcm_hardware = {
	.info			= SNDRV_PCM_INFO_MMAP |
				  SNDRV_PCM_INFO_MMAP_VALID |
				  SNDRV_PCM_INFO_PAUSE |
				  SNDRV_PCM_INFO_RESUME |
				  SNDRV_PCM_INFO_INTERLEAVED,
	.formats		= SNDRV_PCM_FMTBIT_S16_LE,
	.channels_min		= 2,
	.channels_max		= 2,
	.period_bytes_min	= 128,
	.period_bytes_max	= PAGE_SIZE * 2,
	.periods_min		= 1,
	.periods_max		= 8,
	.buffer_bytes_max	= PAGE_SIZE * 8,
	.fifo_size		= 4,
};

int tegra_register_offload_ops(struct tegra_offload_ops *ops)
{
	mutex_lock(&tegra_offload_lock);
	if (!ops) {
		pr_err("Invalid ops pointer.");
		return -EINVAL;
	}

	if (tegra_offload_init_done) {
		pr_err("Offload ops already registered.");
		return -EBUSY;
	}
	memcpy(&offload_ops, ops, sizeof(offload_ops));
	tegra_offload_init_done = 1;
	mutex_unlock(&tegra_offload_lock);

	pr_info("succefully registered offload ops");
	return 0;
}
EXPORT_SYMBOL_GPL(tegra_register_offload_ops);

/* Compress playback related APIs */
static void tegra_offload_compr_fragment_elapsed(void *arg)
{
	struct snd_compr_stream *stream = arg;

	if (stream)
		snd_compr_fragment_elapsed(stream);
}

static int tegra_offload_compr_open(struct snd_compr_stream *stream)
{
	struct snd_soc_pcm_runtime *rtd = stream->device->private_data;
	struct device *dev = rtd->platform->dev;
	struct tegra_offload_compr_data *data;
	int ret = 0;

	dev_vdbg(dev, "%s", __func__);

	if (!tegra_offload_init_done) {
		dev_err(dev, "Offload interface is not registered");
		return -ENODEV;
	}

	if (!stream->device->dev)
		stream->device->dev = dev;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data) {
		dev_vdbg(dev, "Failed to allocate tegra_offload_compr_data.");
		return -ENOMEM;
	}
	data->ops = &offload_ops.compr_ops;

	ret = data->ops->stream_open(&data->stream_id);
	if (ret < 0) {
		dev_err(dev, "Failed to open offload stream. err %d", ret);
		return ret;
	}

	stream->runtime->private_data = data;
	return 0;
}

static int tegra_offload_compr_free(struct snd_compr_stream *stream)
{
	struct device *dev = stream->device->dev;
	struct tegra_offload_compr_data *data = stream->runtime->private_data;

	dev_vdbg(dev, "%s", __func__);

	data->ops->stream_close(data->stream_id);
	devm_kfree(dev, data);
	return 0;
}

static int tegra_offload_compr_set_params(struct snd_compr_stream *stream,
			struct snd_compr_params *params)
{
	struct device *dev = stream->device->dev;
	struct tegra_offload_compr_data *data = stream->runtime->private_data;
	struct snd_soc_pcm_runtime *rtd = stream->device->private_data;
	struct tegra_pcm_dma_params *dmap;
	struct tegra_offload_compr_params offl_params;
	int ret = 0;

	dev_vdbg(dev, "%s", __func__);

	dmap = rtd->cpu_dai->playback_dma_data;
	if (!dmap) {
		struct snd_soc_dpcm *dpcm;

		list_for_each_entry(dpcm,
			&rtd->dpcm[SNDRV_PCM_STREAM_PLAYBACK].be_clients,
			list_be) {
			struct snd_soc_pcm_runtime *be = dpcm->be;
			struct snd_pcm_substream *be_substream =
				snd_soc_dpcm_get_substream(be,
					SNDRV_PCM_STREAM_PLAYBACK);

			dmap = snd_soc_dai_get_dma_data(be->cpu_dai,
							be_substream);
			if (!dmap) {
				dev_err(dev, "Failed to get DMA params.");
				return -ENODEV;
			}
			/* TODO : Multiple BE to single FE not yet supported */
			break;
		}
	}
	offl_params.codec_type = params->codec.id;
	offl_params.bits_per_sample = 16;
	offl_params.rate = snd_pcm_rate_bit_to_rate(params->codec.sample_rate);
	offl_params.channels = params->codec.ch_in;
	offl_params.fragment_size = params->buffer.fragment_size;
	offl_params.fragments = params->buffer.fragments;
	offl_params.dma_params.addr = dmap->addr;
	offl_params.dma_params.width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	offl_params.dma_params.req_sel = dmap->req_sel;
	offl_params.dma_params.max_burst = 4;
	offl_params.fragments_elapsed_cb = tegra_offload_compr_fragment_elapsed;
	offl_params.fragments_elapsed_args = (void *)stream;

	ret = data->ops->set_stream_params(data->stream_id, &offl_params);
	if (ret < 0) {
		dev_err(dev, "Failed to set offload params. ret %d", ret);
		return ret;
	}
	memcpy(&data->codec, &params->codec, sizeof(struct snd_codec));
	return 0;
}

static int tegra_offload_compr_get_params(struct snd_compr_stream *stream,
			struct snd_codec *codec)
{
	struct device *dev = stream->device->dev;
	struct tegra_offload_compr_data *data = stream->runtime->private_data;

	dev_vdbg(dev, "%s", __func__);

	memcpy(codec, &data->codec, sizeof(struct snd_codec));
	return 0;
}

static int tegra_offload_compr_trigger(struct snd_compr_stream *stream, int cmd)
{
	struct device *dev = stream->device->dev;
	struct tegra_offload_compr_data *data = stream->runtime->private_data;

	dev_vdbg(dev, "%s : cmd %d", __func__, cmd);

	data->ops->set_stream_state(data->stream_id, cmd);
	return 0;
}

static int tegra_offload_compr_pointer(struct snd_compr_stream *stream,
			struct snd_compr_tstamp *tstamp)
{
	struct device *dev = stream->device->dev;
	struct tegra_offload_compr_data *data = stream->runtime->private_data;

	dev_vdbg(dev, "%s", __func__);

	return data->ops->get_stream_position(data->stream_id, tstamp);
}

static int tegra_offload_compr_copy(struct snd_compr_stream *stream,
			char __user *buf, size_t count)
{
	struct device *dev = stream->device->dev;
	struct tegra_offload_compr_data *data = stream->runtime->private_data;

	dev_vdbg(dev, "%s : bytes %d", __func__, (int)count);

	return data->ops->write(data->stream_id, buf, count);
}

static int tegra_offload_compr_get_caps(struct snd_compr_stream *stream,
			struct snd_compr_caps *caps)
{
	struct device *dev = stream->device->dev;
	struct tegra_offload_compr_data *data = stream->runtime->private_data;
	int ret = 0;

	dev_vdbg(dev, "%s", __func__);

	caps->direction = stream->direction;
	ret = data->ops->get_caps(caps);
	if (ret < 0) {
		dev_err(dev, "Failed to get compr caps. ret %d", ret);
		return ret;
	}
	return 0;
}

static int tegra_offload_compr_codec_caps(struct snd_compr_stream *stream,
			struct snd_compr_codec_caps *codec_caps)
{
	struct device *dev = stream->device->dev;
	struct tegra_offload_compr_data *data = stream->runtime->private_data;
	int ret = 0;

	dev_vdbg(dev, "%s", __func__);

	if (!codec_caps->codec)
		codec_caps->codec = data->codec.id;

	ret = data->ops->get_codec_caps(codec_caps);
	if (ret < 0) {
		dev_err(dev, "Failed to get codec caps. ret %d", ret);
		return ret;
	}
	return 0;
}

static struct snd_compr_ops tegra_compr_ops = {

	.open = tegra_offload_compr_open,
	.free = tegra_offload_compr_free,
	.set_params = tegra_offload_compr_set_params,
	.get_params = tegra_offload_compr_get_params,
	.trigger = tegra_offload_compr_trigger,
	.pointer = tegra_offload_compr_pointer,
	.copy = tegra_offload_compr_copy,
	.get_caps = tegra_offload_compr_get_caps,
	.get_codec_caps = tegra_offload_compr_codec_caps,
};

/* PCM playback related APIs */
static void tegra_offload_pcm_period_elapsed(void *arg)
{
	struct snd_pcm_substream *substream = arg;

	if (substream)
		snd_pcm_period_elapsed(substream);
}

static int tegra_offload_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct device *dev = rtd->platform->dev;
	struct tegra_offload_pcm_data *data;
	int ret = 0;

	dev_vdbg(dev, "%s", __func__);

	if (!tegra_offload_init_done) {
		dev_err(dev, "Offload interface is not registered");
		return -ENODEV;
	}

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data) {
		dev_vdbg(dev, "Failed to allocate tegra_offload_pcm_data.");
		return -ENOMEM;
	}

	/* Set HW params now that initialization is complete */
	snd_soc_set_runtime_hwparams(substream, &tegra_offload_pcm_hardware);

	/* Ensure period size is multiple of 4 */
	ret = snd_pcm_hw_constraint_step(substream->runtime, 0,
		SNDRV_PCM_HW_PARAM_PERIOD_BYTES, 0x4);
	if (ret) {
		dev_err(dev, "failed to set constraint %d\n", ret);
		return ret;
	}
	data->ops = &offload_ops.pcm_ops;

	ret = data->ops->stream_open(&data->stream_id);
	if (ret < 0) {
		dev_err(dev, "Failed to open offload stream. err %d", ret);
		return ret;
	}
	 offload_ops.device_ops.set_hw_rate(48000);
	substream->runtime->private_data = data;
	return 0;
}

static int tegra_offload_pcm_close(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct device *dev = rtd->platform->dev;
	struct tegra_offload_pcm_data *data = substream->runtime->private_data;

	dev_vdbg(dev, "%s", __func__);

	data->ops->stream_close(data->stream_id);
	devm_kfree(dev, data);
	return 0;
}

static int tegra_offload_pcm_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct device *dev = rtd->platform->dev;
	struct tegra_offload_pcm_data *data = substream->runtime->private_data;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	struct tegra_pcm_dma_params *dmap;
	struct tegra_offload_pcm_params offl_params;
	int ret = 0;

	dev_vdbg(dev, "%s", __func__);

	dmap = snd_soc_dai_get_dma_data(rtd->cpu_dai, substream);
	if (!dmap) {
		struct snd_soc_dpcm *dpcm;

		list_for_each_entry(dpcm,
			&rtd->dpcm[substream->stream].be_clients, list_be) {
			struct snd_soc_pcm_runtime *be = dpcm->be;
			struct snd_pcm_substream *be_substream =
				snd_soc_dpcm_get_substream(be,
						substream->stream);

			dmap = snd_soc_dai_get_dma_data(be->cpu_dai,
							be_substream);
			if (!dmap) {
				dev_err(dev, "Failed to get DMA params.");
				return -ENODEV;
			}
			/* TODO : Multiple BE to single FE not yet supported */
			break;
		}
	}

	offl_params.bits_per_sample =
		snd_pcm_format_width(params_format(params));
	offl_params.rate = params_rate(params);
	offl_params.channels = params_channels(params);
	offl_params.buffer_size = params_buffer_bytes(params);
	offl_params.period_size = params_period_size(params) *
		((offl_params.bits_per_sample >> 3) * offl_params.channels);

	offl_params.dma_params.addr = dmap->addr;
	offl_params.dma_params.width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	offl_params.dma_params.req_sel = dmap->req_sel;
	offl_params.dma_params.max_burst = 4;

	offl_params.source_buf.virt_addr = buf->area;
	offl_params.source_buf.phys_addr = buf->addr;
	offl_params.source_buf.bytes = buf->bytes;

	offl_params.period_elapsed_cb = tegra_offload_pcm_period_elapsed;
	offl_params.period_elapsed_args = (void *)substream;

	ret = data->ops->set_stream_params(data->stream_id, &offl_params);
	if (ret < 0) {
		dev_err(dev, "Failed to set avp params. ret %d", ret);
		return ret;
	}
	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	return 0;
}

static int tegra_offload_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct device *dev = rtd->platform->dev;

	dev_vdbg(dev, "%s", __func__);

	snd_pcm_set_runtime_buffer(substream, NULL);
	return 0;
}

static int tegra_offload_pcm_trigger(struct snd_pcm_substream *substream,
				     int cmd)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct device *dev = rtd->platform->dev;
	struct tegra_offload_pcm_data *data = substream->runtime->private_data;

	dev_vdbg(dev, "%s : cmd %d", __func__, cmd);

	data->ops->set_stream_state(data->stream_id, cmd);
	if ((cmd == SNDRV_PCM_TRIGGER_STOP) ||
	    (cmd == SNDRV_PCM_TRIGGER_SUSPEND) ||
	    (cmd == SNDRV_PCM_TRIGGER_PAUSE_PUSH))
		data->appl_ptr = 0;

	return 0;
}

static snd_pcm_uframes_t tegra_offload_pcm_pointer(
		struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct device *dev = rtd->platform->dev;
	struct tegra_offload_pcm_data *data = substream->runtime->private_data;
	size_t position;

	dev_vdbg(dev, "%s", __func__);

	position = data->ops->get_stream_position(data->stream_id);
	return bytes_to_frames(substream->runtime, position);
}

static int tegra_offload_pcm_ack(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct tegra_offload_pcm_data *data = runtime->private_data;
	int data_size = runtime->control->appl_ptr - data->appl_ptr;

	if (data_size < 0)
		data_size += runtime->boundary;

	data->ops->data_ready(data->stream_id,
		frames_to_bytes(runtime, data_size));
	data->appl_ptr = runtime->control->appl_ptr;
	return 0;
}


static const struct snd_soc_dapm_widget tegra_offload_widgets[] = {
	/* FrontEnd DAIs */
	SND_SOC_DAPM_AIF_IN("offload-pcm-playback", "pcm-playback", 0,
		0/*wreg*/, 0/*wshift*/, 0/*winvert*/),
	SND_SOC_DAPM_AIF_IN("offload-compr-playback", "compr-playback", 0,
		0/*wreg*/, 0/*wshift*/, 0/*winvert*/),

	/* BackEnd DAIs */
	SND_SOC_DAPM_AIF_OUT("I2S0_OUT", "tegra30-i2s.0 Playback", 0,
		0/*wreg*/, 0/*wshift*/, 0/*winvert*/),
	SND_SOC_DAPM_AIF_OUT("I2S1_OUT", "tegra30-i2s.1 Playback", 0,
		0/*wreg*/, 0/*wshift*/, 0/*winvert*/),
	SND_SOC_DAPM_AIF_OUT("I2S2_OUT", "tegra30-i2s.2 Playback", 0,
		0/*wreg*/, 0/*wshift*/, 0/*winvert*/),
	SND_SOC_DAPM_AIF_OUT("I2S3_OUT", "tegra30-i2s.3 Playback", 0,
		0/*wreg*/, 0/*wshift*/, 0/*winvert*/),
	SND_SOC_DAPM_AIF_OUT("I2S4_OUT", "tegra30-i2s.4 Playback", 0,
		0/*wreg*/, 0/*wshift*/, 0/*winvert*/),

};

static struct snd_pcm_ops tegra_pcm_ops = {
	.open		= tegra_offload_pcm_open,
	.close		= tegra_offload_pcm_close,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= tegra_offload_pcm_hw_params,
	.hw_free	= tegra_offload_pcm_hw_free,
	.trigger	= tegra_offload_pcm_trigger,
	.pointer	= tegra_offload_pcm_pointer,
	.ack		= tegra_offload_pcm_ack,
};

static void tegra_offload_dma_free(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	struct tegra_offload_mem mem;

	substream = pcm->streams[stream].substream;
	if (!substream)
		return;

	buf = &substream->dma_buffer;
	if (!buf->area)
		return;

	mem.dev = buf->dev.dev;
	mem.bytes = buf->bytes;
	mem.virt_addr = buf->area;
	mem.phys_addr = buf->addr;

	offload_ops.device_ops.free_shared_mem(&mem);
	buf->area = NULL;
}

static u64 tegra_dma_mask = DMA_BIT_MASK(32);
static int tegra_offload_dma_allocate(struct snd_soc_pcm_runtime *rtd,
			int stream, size_t size)
{
	struct snd_card *card = rtd->card->snd_card;
	struct snd_pcm *pcm = rtd->pcm;
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	struct tegra_offload_mem mem;
	int ret = 0;

	if (!card->dev->dma_mask)
		card->dev->dma_mask = &tegra_dma_mask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = DMA_BIT_MASK(32);

	ret = offload_ops.device_ops.alloc_shared_mem(&mem, size);
	if (ret < 0) {
		dev_err(pcm->card->dev, "Failed to allocate memory");
		return -ENOMEM;
	}
	buf->area = mem.virt_addr;
	buf->addr = mem.phys_addr;
	buf->private_data = NULL;
	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = mem.dev;
	buf->bytes = mem.bytes;
	return 0;
}

static int tegra_offload_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	struct device *dev = rtd->platform->dev;

	dev_vdbg(dev, "%s", __func__);

	return tegra_offload_dma_allocate(rtd , SNDRV_PCM_STREAM_PLAYBACK,
				tegra_offload_pcm_hardware.buffer_bytes_max);
}

static void tegra_offload_pcm_free(struct snd_pcm *pcm)
{
	tegra_offload_dma_free(pcm, SNDRV_PCM_STREAM_PLAYBACK);
	pr_debug("%s", __func__);
}

static int tegra_offload_pcm_probe(struct snd_soc_platform *platform)
{
	pr_debug("%s", __func__);

	snd_soc_dapm_new_controls(&platform->dapm, tegra_offload_widgets,
					ARRAY_SIZE(tegra_offload_widgets));

	snd_soc_dapm_new_widgets(&platform->dapm);
	platform->dapm.idle_bias_off = 1;
	return 0;
}

unsigned int tegra_offload_pcm_read(struct snd_soc_platform *platform,
		unsigned int reg)
{
	return 0;
}

int tegra_offload_pcm_write(struct snd_soc_platform *platform, unsigned int reg,
		unsigned int val)
{
	return 0;
}

static struct snd_soc_platform_driver tegra_offload_platform = {
	.ops		= &tegra_pcm_ops,
	.compr_ops	= &tegra_compr_ops,
	.pcm_new	= tegra_offload_pcm_new,
	.pcm_free	= tegra_offload_pcm_free,
	.probe		= tegra_offload_pcm_probe,
	.read		= tegra_offload_pcm_read,
	.write		= tegra_offload_pcm_write,
};

static struct snd_soc_dai_driver tegra_offload_dai[] = {
	[PCM_OFFLOAD_DAI] = {
		.name = "tegra-offload-pcm",
		.id = 0,
		.playback = {
			.stream_name = "pcm-playback",
			.channels_min = 2,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
	},
	[COMPR_OFFLOAD_DAI] = {
		.name = "tegra-offload-compr",
		.id = 0,
		.compress_dai = 1,
		.playback = {
			.stream_name = "compr-playback",
			.channels_min = 2,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
	}
};

static const struct snd_soc_component_driver tegra_offload_component = {
	.name		= "tegra-offload",
};

static int tegra_offload_probe(struct platform_device *pdev)
{
	int ret;

	pr_debug("tegra30_avp_pcm platform probe started\n");

	ret = snd_soc_register_platform(&pdev->dev, &tegra_offload_platform);
	if (ret) {
		dev_err(&pdev->dev, "Could not register platform: %d\n", ret);
		goto err;
	}

	ret = snd_soc_register_component(&pdev->dev, &tegra_offload_component,
			tegra_offload_dai, ARRAY_SIZE(tegra_offload_dai));
	if (ret) {
		dev_err(&pdev->dev, "Could not register component: %d\n", ret);
		goto err_unregister_platform;
	}
	pr_info("tegra_offload_platform probe successfull.");
	return 0;

err_unregister_platform:
	snd_soc_unregister_platform(&pdev->dev);
err:
	return ret;
}

static int tegra_offload_remove(struct platform_device *pdev)
{
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

static const struct of_device_id tegra_offload_of_match[] = {
	{ .compatible = "nvidia,tegra-offload", },
	{},
};

static struct platform_driver tegra_offload_driver = {
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = tegra_offload_of_match,
	},
	.probe = tegra_offload_probe,
	.remove = tegra_offload_remove,
};
module_platform_driver(tegra_offload_driver);

MODULE_AUTHOR("Sumit Bhattacharya <sumitb@nvidia.com>");
MODULE_DESCRIPTION("Tegra offload platform driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRV_NAME);
MODULE_DEVICE_TABLE(of, tegra_offload_of_match);
