export default function refreshAfterUpdate() {
	window.loadingIndicator?.classList.remove('hidden')
	setTimeout(() => window.location.reload(), [5000])
}
