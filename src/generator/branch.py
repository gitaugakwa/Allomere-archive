import librosa
from itertools import tee, islice, chain
import numpy as np

# Move away from tf since there's no model currently
import tensorflow as tf
import tensorflow_io as tfio
import asyncio

DURATION = 3 * 60 # 3 minute songs
SAMPLE_RATE = 44100 # my FLAC sample rate
CHANNELS = 2
CHUNK = 1024

async def get_mel_spectrogram(waveform, pad: bool = True, input_samples= DURATION * SAMPLE_RATE, frame_step = 128, frame_length=255 ):
  # Zero-padding for an audio waveform with less than 16,000 samples.
  channels = waveform.shape[1]
  input_len = input_samples
  waveform = waveform[:input_len]
  
  # Cast the waveform tensors' dtype to float32.
  waveform = tf.cast(waveform, dtype=tf.float32)
  # Concatenate the waveform with `zero_padding`, which ensures all audio
  # clips are of the same length.
  tfio.audio.melscale()
  spectrogram = []
  for channel in range(channels):
    data = waveform[:,channel]
    if pad:
        zero_padding = tf.zeros(
            input_samples - waveform.shape[0],
            dtype=tf.float32)
        data = tf.concat([waveform[:,channel], zero_padding], 0)
    # Convert the waveform to a spectrogram via a STFT.
    spectrogram.append(tf.signal.stft(
        data, frame_length=frame_length, frame_step=frame_step))
    # Obtain the magnitude of the STFT.
    spectrogram[channel] = tf.abs(spectrogram[channel])
    spectrogram[channel] = spectrogram[channel][..., tf.newaxis]
    # Add a `channels` dimension, so that the spectrogram can be used
    # as image-like input data with convolution layers (which expect
    # shape (`batch_size`, `height`, `width`, `channels`).
    # spectrogram = spectrogram[..., tf.newaxis]
	# shape ('width', 'batch_size', 'height')
  return tf.concat(spectrogram, axis=2)

async def get_spectrogram(waveform, pad: bool = True, input_samples= DURATION * SAMPLE_RATE, frame_step = 128, frame_length=255 ):
  # Zero-padding for an audio waveform with less than 16,000 samples.
  channels = waveform.shape[1]
  input_len = input_samples
  waveform = waveform[:input_len]
  
  # Cast the waveform tensors' dtype to float32.
  waveform = tf.cast(waveform, dtype=tf.float32)
  # Concatenate the waveform with `zero_padding`, which ensures all audio
  # clips are of the same length.
  spectrogram = []
  for channel in range(channels):
    data = waveform[:,channel]
    if pad:
        zero_padding = tf.zeros(
            input_samples - waveform.shape[0],
            dtype=tf.float32)
        data = tf.concat([waveform[:,channel], zero_padding], 0)
    # Convert the waveform to a spectrogram via a STFT.
    spectrogram.append(tf.signal.stft(
        data, frame_length=frame_length, frame_step=frame_step))
    # Obtain the magnitude of the STFT.
    spectrogram[channel] = tf.abs(spectrogram[channel])
    spectrogram[channel] = spectrogram[channel][..., tf.newaxis]
    # Add a `channels` dimension, so that the spectrogram can be used
    # as image-like input data with convolution layers (which expect
    # shape (`batch_size`, `height`, `width`, `channels`).
    # spectrogram = spectrogram[..., tf.newaxis]
	# shape ('width', 'batch_size', 'height')
  return tf.concat(spectrogram, axis=2)


async def get_beat_track(waveforms, sr):
	# print(waveforms)
	# print(waveforms[0].shape)
	# print(waveforms[0])
	tempos = np.array([np.sum(np.squeeze(librosa.beat.tempo(y=waveform.T)),axis=-1)/2 for waveform in waveforms])
	# l_tempo, r_tempo = np.squeeze(librosa.beat.tempo(y=waveforms))
	# print(l_tempo, r_tempo)
	# tempo = math.ceil((l_tempo+r_tempo)/2)

	beat_tracks = np.array([librosa.beat.beat_track(y=librosa.to_mono(waveform.T), sr=sr, tightness=1, hop_length=256, trim=False, units="samples")[1] for waveform in waveforms])
	# print(beat_tracks.shape)
	# print(tempos.shape)

	# time_stamps =librosa.samples_to_time(beat_track, sr=sr)

	# print(np.array(beat_track).reshape((-1,1)).shape)
	# print(beat_track)
	# print(time_stamps)
	# print(tempo)
	return [beat_tracks, tempos]

async def get_cross_similarity_mat(waveform, sr):
	get_beat_track_r = get_beat_track(waveform, sr)
	def previous_and_next(some_iterable):
		prevs, items, nexts = tee(some_iterable, 3)
		prevs = chain([None], prevs)
		nexts = chain(islice(nexts, 1, None), [None])
		return zip(prevs, items, nexts)

	def mse(A,B):
		mean = (np.squeeze(A - B)**2).mean()
		# print(mean)
		return mean
	vmse = np.vectorize(mse, signature='(m),(m)->()')
	def matmse(A,B):
		return vmse(A,B).reshape(-1,1)
	vmatmse = np.vectorize(matmse, signature='(m),(n,m)->(n,1)')

	beat_track, tempo = await get_beat_track_r
	beats = beat_track.shape[0]
	csm_mat = np.zeros((beats, beats))
	for a, (a_sample_start, a_sample_end, _) in enumerate(previous_and_next(beat_track)):
		print(a)
		for b, (b_sample_start, b_sample_end, _) in enumerate(previous_and_next(beat_track[a+1:])):
			csm_mat[a, b+1] = np.mean(librosa.segment.cross_similarity(waveform[a_sample_start:a_sample_end,:].T, waveform[b_sample_start:b_sample_end, :].T, k=2))
	# beat_samples = np.array([np.mean(librosa.segment.cross_similarity(waveform[:,p_sample:sample], waveform[:,sample:n_sample]), axis=0) for p_sample, sample, n_sample in previous_and_next(beat_track)])

	# mse_arrays = [np.concatenate([np.zeros((i,1)),beat_samples[i,:,1],beat_samples[i:,:,1]]) for i in range(beats)]
	return csm_mat.T, beat_track, tempo


async def get_cosine_sim_mat_improved(waveform, sr, padding=5000):
	get_beat_track_r = get_beat_track(waveform, sr)

	def previous_and_next(some_iterable):
		prevs, items, nexts = tee(some_iterable, 3)
		prevs = chain([None], prevs)
		nexts = chain(islice(nexts, 1, None), [None])
		return zip(prevs, items, nexts)
	
	def cosine_sim(A,B):
		similarity = (1- (np.dot(A, B)/(np.linalg.norm(A)*np.linalg.norm(B)))).mean()
		# mean = (np.squeeze(A - B)**2).mean()
		# print(mean)
		return similarity
	vcosine_sim = np.vectorize(cosine_sim, signature='(m),(m)->()')
	def matcosine_sim(A,B):
		return vcosine_sim(A,B).reshape(-1,1)
	vmatcosine_sim = np.vectorize(matcosine_sim, signature='(m),(n,m)->(n,1)')


	beat_track, tempo = await get_beat_track_r
	beats = beat_track.shape[0]
	samples = waveform.shape[0]
	beat_samples = np.array([np.mean(await get_spectrogram(waveform[max(sample-padding, 0):min(sample+padding, samples),:], pad=False), axis=0) for _, sample, _ in previous_and_next(beat_track)])
	# print(beat_samples.shape)

	cosine_sim_arrays = [np.concatenate([np.zeros((i,1)),vmatcosine_sim(beat_samples[i,:,1],beat_samples[i:,:,1])]) for i in range(beats)]

	cosine_sim_mat = np.column_stack(cosine_sim_arrays)
	cosine_sim_mat += cosine_sim_mat.T
	# print(cosine_sim_mat.shape)
	return cosine_sim_mat,beat_track, tempo


async def get_mse_mat_improved(waveforms, sr, padding=5000):
	get_beat_track_r = get_beat_track(waveforms, sr)

	def previous_and_next(some_iterable):
		prevs, items, nexts = tee(some_iterable, 3)
		prevs = chain([None], prevs)
		nexts = chain(islice(nexts, 1, None), [None])
		return zip(prevs, items, nexts)

	async def get_spectrograms(waveform, beat_track, samples):
		return np.array([np.mean(await get_spectrogram(waveform[max(sample-padding, 0):min(sample+padding, samples),:], pad=False), axis=0) for _, sample, _ in previous_and_next(beat_track)])
	
	def mse(A,B):
		mean = (np.squeeze(A - B)**2).mean()
		# print(mean)
		return mean
	vmse = np.vectorize(mse, signature='(m),(m)->()')
	def matmse(A,B):
		return vmse(A,B).reshape(-1,1)
	vmatmse = np.vectorize(matmse, signature='(m),(n,m)->(n,1)')


	beat_tracks, tempo = await get_beat_track_r
	beats = [beat_track.shape[0] for beat_track in beat_tracks]
	# print(beats)
	# print(waveforms.shape)
	samples = [waveform.shape[0] for waveform in waveforms]
	beat_spectrograms = np.array([await get_spectrograms(waveform,beat_track, samples) for waveform,beat_track, samples in zip(waveforms, beat_tracks, samples)])
	# beat_spectrogram_coroutines = [get_spectrograms(waveform,beat_track, samples) for waveform,beat_track, samples in zip(waveforms, beat_tracks, samples)]
	# beat_spectrograms = np.array(await asyncio.gather(*beat_spectrogram_coroutines))
	# print(beat_spectrograms[0].shape)
	# print(beat_spectrograms[1].shape)
	spectrograms = np.concatenate([spectrogram[:,:,1] for spectrogram in beat_spectrograms])

	if(len(waveforms) == 2):
		mse_arrays = [np.concatenate([np.zeros((i,1)),vmatmse(spectrograms[i,:], spectrograms[i:,:])]) for i in range(np.sum(beats))]
	else:
		mse_arrays = [np.concatenate([np.zeros((i,1)),vmatmse(beat_spectrograms[0][i,:,1],beat_spectrograms[0][i:,:,1])]) for i in range(np.sum(beats))]

	mse_mat = np.column_stack(mse_arrays)
	mse_mat += mse_mat.T
	# print(mse_mat.shape)
	return mse_mat,beat_tracks, tempo


async def get_mse_mat(waveform, sr):
	get_beat_track_r = get_beat_track(waveform, sr)

	def previous_and_next(some_iterable):
		prevs, items, nexts = tee(some_iterable, 3)
		prevs = chain([None], prevs)
		nexts = chain(islice(nexts, 1, None), [None])
		return zip(prevs, items, nexts)
	
	def mse(A,B):
		mean = (np.squeeze(A - B)**2).mean()
		# print(mean)
		return mean
	vmse = np.vectorize(mse, signature='(m),(m)->()')
	def matmse(A,B):
		return vmse(A,B).reshape(-1,1)
	vmatmse = np.vectorize(matmse, signature='(m),(n,m)->(n,1)')


	beat_track, tempo = await get_beat_track_r
	beats = beat_track.shape[0]
	beat_samples = np.array([np.mean(await get_spectrogram(waveform[p_sample:sample, :], pad=False), axis=0) for p_sample, sample, _ in previous_and_next(beat_track)])
	# print(beat_samples.shape)

	mse_arrays = [np.concatenate([np.zeros((i,1)),vmatmse(beat_samples[i,:,1],beat_samples[i:,:,1])]) for i in range(beats)]

	mse_mat = np.column_stack(mse_arrays)
	# print(mse_mat.shape)
	return mse_mat,beat_track, tempo



async def get_beats_map(waveforms, sr, algo, padding, quality, quantity, gap ):
	# mse_mat, beat_track,tempo=await get_cross_similarity_mat(waveform,sr)
	# mse_mat, beat_track,tempo=await get_mse_mat_improved(waveform,sr)
	# mse_mat, beat_track,tempo=await get_mse_mat_improved(waveform,sr, padding)

	if(algo == "mse"):
		dif_mat, beat_tracks,tempo=await get_mse_mat_improved(waveforms,sr, padding)
	if(algo == "cossim"):
		dif_mat, beat_tracks,tempo=await get_cosine_sim_mat_improved(waveforms,sr, padding)

	# print(dif_mat)
	beats = [beat_track.shape[0] for beat_track in beat_tracks]
	# beats = beat_track.shape[0]

	source_beat = np.tile(np.arange(np.sum(beats)), (np.sum(beats), 1)).T
	end_beat = source_beat.T
	# print(dif_mat.shape)
	# print(source_beat.shape)
	# print(end_beat.shape)

	beat_to_beat_mse = np.stack([source_beat, dif_mat, end_beat], axis=2)
	# print(beat_to_beat_mse.shape)
	beat_to_beat_map = beat_to_beat_mse[beat_to_beat_mse[:,:, 1] > 0]
	beat_to_beat_map_sorted = beat_to_beat_map[beat_to_beat_map[:, 1].argsort()]

	# best_match_map_long = beat_to_beat_map_sorted[(beat_to_beat_map_sorted[:, 0] - beat_to_beat_map_sorted[:, 2]) > gap]
	best_match_map_filtered = beat_to_beat_map_sorted[abs(beat_to_beat_map_sorted[:, 0] -beat_to_beat_map_sorted[:, 2]) > gap]

	if quality >= 1:
		best_match_map_filtered = best_match_map_filtered[((best_match_map_filtered[:, 1] / best_match_map_filtered[-1, 1]) * 100) <= (100-quality)]
	else:
		best_match_map_filtered = best_match_map_filtered[best_match_map_filtered[:, 1] <= quality]
		
	forward_map = best_match_map_filtered[(best_match_map_filtered[:, 0] - best_match_map_filtered[:, 2]) < 0]
	backward_map = best_match_map_filtered[(best_match_map_filtered[:, 0] - best_match_map_filtered[:, 2]) > 0]

	if(quantity):
		# print(best_match_map_filtered.shape)
		forward_map = forward_map[:quantity, :]
		backward_map = backward_map[:quantity, :]
		best_match_map_filtered = np.concatenate([forward_map, backward_map])

	
	return beat_tracks,forward_map,backward_map, best_match_map_filtered
