#pragma once
#include "stdint.h"

//Opaque model data container
typedef struct model_iface_data_t model_iface_data_t;

//Set wake words recognition operating mode
//The probability of being wake words is increased with increasing mode,
//As a consequence also the false alarm rate goes up
typedef enum {
    DET_MODE_90 = 0,  //Normal, response accuracy rate about 90%
    DET_MODE_95       //Aggressive, response accuracy rate about 95%
} det_mode_t;

/**
 * @brief Easy function type to initialze a model instance with a detection mode
 *
 * @param det_mode The wake words detection mode to trigger wake words, the range of det_threshold is 0.5~0.9999
 * @returns Handle to the model data
 */
typedef model_iface_data_t *(*esp_sr_iface_op_create_t)(det_mode_t det_mode);


/**
 * @brief Callback function type to fetch the amount of samples that need to be passed to the detect function
 *
 * Every speech recognition model processes a certain number of samples at the same time. This function
 * can be used to query that amount. Note that the returned amount is in 16-bit samples, not in bytes.
 *
 * @param model The model object to query
 * @return The amount of samples to feed the detect function
 */
typedef int (*esp_sr_iface_op_get_samp_chunksize_t)(model_iface_data_t *model);


/**
 * @brief Get the sample rate of the samples to feed to the detect function
 *
 * @param model The model object to query
 * @return The sample rate, in hz
 */
typedef int (*esp_sr_iface_op_get_samp_rate_t)(model_iface_data_t *model);

/**
 * @brief Get the number of wake words
 *
 * @param model The model object to query
 * @returns the number of wake words
 */
typedef int (*esp_sr_iface_op_get_word_num_t)(model_iface_data_t *model);

/**
 * @brief Get the name of wake word by index
 *
 * @Warning The index of wake word start with 1

 * @param model The model object to query
 * @param word_index The index of wake word
 * @returns the detection threshold
 */
typedef char *(*esp_sr_iface_op_get_word_name_t)(model_iface_data_t *model, int word_index);

/**
 * @brief Set the detection threshold to manually abjust the probability
 *
 * @param model The model object to query
 * @param det_treshold The threshold to trigger wake words, the range of det_threshold is 0.5~0.9999
 * @param word_index The index of wake word
 * @return 0: setting failed, 1: setting success
 */
typedef int (*esp_sr_iface_op_set_det_threshold_t)(model_iface_data_t *model, float det_threshold, int word_index);

/**
 * @brief Get the wake word detection threshold of different modes
 *
 * @param model The model object to query
 * @param det_mode The wake words recognition operating mode
 * @param word_index The index of wake word
 * @returns the detection threshold
 */
typedef float (*esp_sr_iface_op_get_det_threshold_t)(model_iface_data_t *model, det_mode_t det_mode, int word_index);

/**
 * @brief Feed samples of an audio stream to the speech recognition model and detect if there is a keyword found.
 *
 * @Warning The index of wake word start with 1, 0 means no wake words is detected.
 *
 * @param model The model object to query
 * @param samples An array of 16-bit signed audio samples. The array size used can be queried by the
 *        get_samp_chunksize function.
 * @return The index of wake words, return 0 if no wake word is detected, else the index of the wake words.
 */
typedef int (*esp_sr_iface_op_detect_t)(model_iface_data_t *model, int16_t *samples);

/**
 * @brief Destroy a speech recognition model
 *
 * @param model Model object to destroy
 */
typedef void (*esp_sr_iface_op_destroy_t)(model_iface_data_t *model);


/**
 * This structure contains the functions used to do operations on a speech recognition model.
 */
typedef struct {
    esp_sr_iface_op_create_t create;
    esp_sr_iface_op_get_samp_chunksize_t get_samp_chunksize;
    esp_sr_iface_op_get_samp_rate_t get_samp_rate;
    esp_sr_iface_op_get_word_num_t get_word_num;
    esp_sr_iface_op_get_word_name_t get_word_name;
    esp_sr_iface_op_set_det_threshold_t set_det_threshold;
    esp_sr_iface_op_get_det_threshold_t get_det_threshold_by_mode;
    esp_sr_iface_op_detect_t detect;
    esp_sr_iface_op_destroy_t destroy;
} esp_sr_iface_t;


/* esp_sr example

#include "esp_sr_iface.h"
#include "esp_sr_models.h"

#if CONFIG_SR_MODEL_WN3_QUANT
#define SR_MODEL esp_sr_wakenet3_quantized
#define SR_MODEL_COEFF_GETTER get_coeff_wakeNet3_model_float
#else
#error No valid neural network model selected.
#endif // END of CONFIG_SR_MODEL_WN3_QUANT

static const esp_sr_iface_t *model = &SR_MODEL;
static model_iface_data_t *model_data;

model_data = model->create(DET_MODE_90);
int audio_chunksize = model->get_samp_chunksize(model_data);
int frequency = model->get_samp_rate(model_data);
char *wakeupBuf = malloc(audio_chunksize *sizeof(short));
while (1)
{
    // read i2s data to wakeupBuf
    int ret = model->detect(model_data, (int16_t *)wakeupBuf);
    if (ret == 1) {
        ESP_LOGW(TAG, "### spot keyword ###");
    }
}

*/