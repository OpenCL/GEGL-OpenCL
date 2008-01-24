#ifndef _LENS_CORRECT_H_
#define _LENS_CORRECT_H_

/* Struct containing the correction parameters a,b,c,d for a lens color channel.
 * These parameters as the same as used in the Panotools system.
 * For a detailed explanation, please consult http://wiki.panotools.org/Lens_correction_model
 *
 * Normally a, b, and c are close to zero, and d is close to one.
 * Note that d is the parameter that's approximately equal to 1 - a - b - c, NOT one of the image
 * shift parameters as used in some GUIs.
 */
typedef struct {
    gfloat a, b, c, d;
} ChannelCorrectionModel;

/* Struct containing all the information required for lens correction.  It includes the total size
 * of the image plus the correction parameters for each color channel.
 */
typedef struct {
    GeglRectangle BB;				/* Bounding box of the imaged area. */
    gfloat cx, cy;				/* Coordinates of lens center within the imaged area.  */
    gfloat rscale;				/* Scale of the image (1/2 of the shortest side). */
    ChannelCorrectionModel red, green, blue;	/* Correction parameters for each color channel. */
} LensCorrectionModel;

#endif
