
if HAVE_AVFORMAT
# List of tests.

TESTS: 

EXTRA_DIST = $(TESTS)

check: vids images

images: 
	for a in *.avi *.mp4 *.ogv ;do \
            GEGL_PATH=$(top_builddir)/operations $(top_builddir)/examples/gegl-video \
            $$a -s 74 -e 74 -of $$a- ;\
        done

vids: $(TESTS) $(top_builddir)/examples/frame-counter
	GEGL_PATH=$(top_builddir)/operations $(top_builddir)/examples/frame-counter \
           --video-codec mpeg4 --video-bit-rate 128 mpeg4-128kb.avi
	GEGL_PATH=$(top_builddir)/operations $(top_builddir)/examples/frame-counter \
           --video-codec mpeg4 --video-bit-rate 512 mpeg4-512kb.avi
	GEGL_PATH=$(top_builddir)/operations $(top_builddir)/examples/frame-counter \
           --video-codec mpeg4 --video-bit-rate 512 512kb.mp4
	GEGL_PATH=$(top_builddir)/operations $(top_builddir)/examples/frame-counter \
           --video-codec mpeg4 --video-bit-rate 128 128kb.mp4
	GEGL_PATH=$(top_builddir)/operations $(top_builddir)/examples/frame-counter \
           --video-codec mpeg4 --fps 12 --video-bit-rate 128 128kb-12fps.mp4
	GEGL_PATH=$(top_builddir)/operations $(top_builddir)/examples/frame-counter \
           --video-codec mpeg4 --fps 100 --video-bit-rate 128 128kb-100fps.mp4
	GEGL_PATH=$(top_builddir)/operations $(top_builddir)/examples/frame-counter \
           --fps 28 --video-bit-rate 512 512kb-28fps.ogv

clean-local:
	rm -f *.png *.avi *.ogv *.mp4 *.webm

endif # HAVE_AVFORMAT
