function Message({ first_name, last_name, text }) {
    return (
        <div className="flex">
            <img className="w-12 h-12 rounded-full" src="profile.jpeg" alt="profile" />
            <div className="ml-4 space-y-2 bg-gray-100  px-4 py-2 max-w-md rounded-xl whitespace-pre-line line-clamp-1">
                <p className="font-semibold">{first_name + (last_name ? " " + last_name : "")}</p>
                <p className="text-lg" style={{ wordWrap: "break-word" }}>
                    {text}
                </p>
            </div>
        </div>
    );
}

export default Message;
