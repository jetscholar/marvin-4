#pragma once
// Keep order consistent with your training / labels.txt
static const char* const KWS_LABELS[] = {
	"marvin",
	"unknown",
	"silence"
};
static const int KWS_NUM_LABELS = sizeof(KWS_LABELS) / sizeof(KWS_LABELS[0]);
